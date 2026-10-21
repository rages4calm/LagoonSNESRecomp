using System.Text.Json;
using Companion.Launcher.Core;

internal static partial class NativeSettings
{
    private static readonly OptionDefinition[] RewardOptions = {
        new("qol.exp-multiplier", "Quality of life", "EXP rewards", OptionKind.Choice, OptionValue.From("1"),
            "Enemy and boss rewards relative to the original game. Does not rewrite your existing EXP. Restart after Apply.",
            Choices: new[] {new ChoiceItem("1","Original (1x)"),new ChoiceItem("1.5","1.5x (previous EXP boost)")}
                .Concat(Enumerable.Range(2,7).Select(n=>new ChoiceItem(n.ToString(),n+"x"))).ToArray()),
        new("qol.gold-multiplier", "Quality of life", "Gold rewards", OptionKind.Choice, OptionValue.From("1"),
            "Enemy and boss gold only. Shop prices, purchases, quest rewards and saved gold are unchanged. Restart after Apply.",
            Choices: Enumerable.Range(1,8).Select(n=>new ChoiceItem(n.ToString(),n==1?"Original (1x)":n+"x")).ToArray())
    };
    private static IEnumerable<string> RewardFeatures(string kind) =>
        (kind=="exp"?new[]{"exp_boost"}:Array.Empty<string>()).Concat(Enumerable.Range(2,7).Select(n=>$"{kind}_{n}x"));
    private static string? RewardFeature(string kind,string value) => value=="1"?null:value=="1.5"?"exp_boost":$"{kind}_{value}x";
    private static string ReadReward(Document mods,string kind)
    {
        var enabled=RewardFeatures(kind).Where(id=>mods.Feature(id)==true).ToArray();
        if(enabled.Length>1)throw new InvalidDataException($"Conflicting {kind.ToUpperInvariant()} reward multipliers. Select only one in native mod state.");
        if(enabled.Length==0)return "1";
        return enabled[0]=="exp_boost"?"1.5":enabled[0].Split('_')[1].TrimEnd('x');
    }
    private static void ReadRewards(SettingsSnapshot snapshot,Document mods)
    {
        foreach(var kind in new[]{"exp","gold"})snapshot.Values[$"qol.{kind}-multiplier"]=OptionValue.From(ReadReward(mods,kind));
    }
    private static IEnumerable<string> SelectedRewardFeatures(IReadOnlyDictionary<string,JsonElement> values)
    {
        foreach(var kind in new[]{"exp","gold"})
            if(values.TryGetValue($"qol.{kind}-multiplier",out var value)&&RewardFeature(kind,value.GetString()!) is {} feature)yield return feature;
    }
    private static void ApplyRewards(SettingsSnapshot current,IReadOnlyDictionary<string,JsonElement> values,Document mods)
    {
        foreach(var kind in new[]{"exp","gold"}){
            var id=$"qol.{kind}-multiplier";
            if(values[id].GetRawText()==current.Values[id].GetRawText())continue;
            var selected=RewardFeature(kind,values[id].GetString()!);
            foreach(var feature in RewardFeatures(kind))
                if(mods.Feature(feature)==true||feature==selected)mods.SetFeature(feature,feature==selected);
        }
    }
    private sealed record KeyboardBinding(string Key,string Label,string Default);
    private static readonly KeyboardBinding[] KeyboardBindings={
        new("a","A","X"),new("b","B / sword","Z"),new("x","X / talk","S"),new("y","Y","A"),
        new("l","L","C"),new("r","R / magic","V"),new("start","Start / menu","Return"),new("select","Select","Right Shift"),
        new("up","Up","Up"),new("down","Down","Down"),new("left","Left","Left"),new("right","Right","Right")
    };
    private static Field Hotkey(string key,string label,string fallback,string help="") => new("hotkey."+key.ToLowerInvariant(),"KeyMap",key,
        new("hotkey."+key.ToLowerInvariant(),"Hotkeys",label,OptionKind.Text,OptionValue.From(fallback),
        help.Length>0?help:"Use an SDL key name, optionally Ctrl+, Alt+ or Shift+. None disables it. Apply and restart."));
    private static readonly Field[] Hotkeys={
        Hotkey("Fullscreen","Fullscreen","Alt+Return"),Hotkey("Reset","Reset game","Ctrl+r"),
        Hotkey("Pause","Pause","Shift+p"),Hotkey("PauseDimmed","Pause with dimmed picture","p"),
        Hotkey("Turbo","Fast forward (hold)","Tab"),Hotkey("WindowBigger","Increase window scale","Ctrl+Up"),
        Hotkey("WindowSmaller","Decrease window scale","Ctrl+Down"),Hotkey("VolumeUp","Master volume up","Keypad +"),
        Hotkey("VolumeDown","Master volume down","Keypad -"),Hotkey("DisplayPerf","Performance display","f"),
        Hotkey("ToggleRenderer","Toggle renderer","r"),Hotkey("ToggleWidescreen","Toggle widescreen","Alt+w"),
        Hotkey("SaveStateMenu","Save-state browser","F11"),Hotkey("Rewind","Rewind","F12"),
        Hotkey("Screenshot","Screenshot","None"),
        Hotkey("Load","Load state slots 1–10",string.Join(", ",Enumerable.Range(1,10).Select(n=>"F"+n)),"Comma-separated keys for consecutive state slots. This is separate from the normal in-game save."),
        Hotkey("Save","Save state slots 1–10",string.Join(", ",Enumerable.Range(1,10).Select(n=>"Shift+F"+n)),"Comma-separated keys for consecutive state slots. This is separate from the normal in-game save.")
    };
    private static readonly string[] PadKeys={"up","down","left","right","select","start","a","b","x","y","l","r"};
    private static readonly string[] PadDefaults={"DpadUp","DpadDown","DpadLeft","DpadRight","Back","Start","B","A","Y","X","Lb","Rb"};
    private static readonly string[] PadButtons={"A","B","X","Y","Lb","Rb","Back","Start","L3","R3","DpadUp","DpadDown","DpadLeft","DpadRight","L1","R1","Guide"};
    private static readonly OptionDefinition[] PadOptions=PadKeys.Select((key,n)=>new OptionDefinition("pad."+key,"Controller", "SNES "+key.ToUpperInvariant(),OptionKind.Choice,OptionValue.From(PadDefaults[n]),
        "Player one controller assignment. L2 opens the map; R2 opens in-game settings. Both triggers are reserved. Existing controller GUID selection is preserved.",
        Choices:PadButtons.Select(b=>new ChoiceItem(b,b)).ToArray())).ToArray();
    private static readonly OptionDefinition[] InputOptions=Hotkeys.Select(h=>h.Option)
        .Concat(KeyboardBindings.Select(k=>new OptionDefinition("keyboard."+k.Key,"Keyboard",k.Label,OptionKind.Text,OptionValue.From(k.Default),"SDL key name, e.g. Z, Return or Right Shift. None disables it. Apply and restart.")))
        .Concat(PadOptions).ToArray();
    private static string KeybindsPath(IReadOnlyDictionary<string,JsonElement> values)=>Path.Combine(Path.GetDirectoryName(values["engine.path"].GetString()!)!,"keybinds.ini");
    private static void ReadInput(SettingsSnapshot snapshot,Document ini,Document keys)
    {
        foreach(var field in Hotkeys)snapshot.Values[field.Id]=OptionValue.From(ini.IniValue("KeyMap",field.Key)??field.Option.DefaultValue.GetString()!);
        foreach(var key in KeyboardBindings)snapshot.Values["keyboard."+key.Key]=OptionValue.From(keys.IniValue("player1",key.Key)??key.Default);
        var mapping=ini.IniValue("GamepadMap","Controls")?.Split(',').Select(x=>x.Trim()).ToArray()??PadDefaults;
        if(mapping.Length!=12)throw new InvalidDataException("Native controller Controls must contain twelve assignments.");
        for(int n=0;n<12;n++)snapshot.Values["pad."+PadKeys[n]]=OptionValue.From(mapping[n]);
    }
    private static void ValidateInput(IReadOnlyDictionary<string,JsonElement> values)
    {
        foreach(var option in InputOptions){
            if(!values.TryGetValue(option.Id,out var value))continue;
            var text=value.GetString()??"";
            if(text.Length>320||text.Any(c=>char.IsControl(c)||c is '[' or ']' or ';' or '#' or '"'))
                throw new InvalidDataException("Invalid binding for "+option.Label+". Use a single line of native key names.");
        }
    }
    private static void ApplyInput(SettingsSnapshot current,IReadOnlyDictionary<string,JsonElement> values,Document ini,Document keys)
    {
        foreach(var field in Hotkeys)if(values[field.Id].GetRawText()!=current.Values[field.Id].GetRawText())ini.SetIni("KeyMap",field.Key,values[field.Id].GetString()!);
        foreach(var key in KeyboardBindings){var id="keyboard."+key.Key;if(values[id].GetRawText()!=current.Values[id].GetRawText())keys.SetIni("player1",key.Key,values[id].GetString()!);}
        if(PadKeys.Any(key=>values["pad."+key].GetRawText()!=current.Values["pad."+key].GetRawText()))
            ini.SetIni("GamepadMap","Controls",string.Join(", ",PadKeys.Select(key=>values["pad."+key].GetString())));
    }
}
