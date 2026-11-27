using System.Security.Cryptography;
using System.Text.Json;
using Companion.Launcher.Core;

internal static class X68000Preferences
{
    private sealed record Setting(OptionDefinition Option, string IniSection, string Key);
    private static readonly Setting[] Fields =
    {
        new(new("x68.display", "Display", "Window mode", OptionKind.Choice, OptionValue.From("0"),
            "Borderless fullscreen uses the monitor's existing resolution. Reopen the game after Apply.",
            Choices: new[] { new ChoiceItem("0", "Windowed"), new ChoiceItem("1", "Borderless · pixel perfect"),
                             new ChoiceItem("2", "Borderless · fit"), new ChoiceItem("3", "Borderless · stretch") }),
            "X68000", "display_mode"),
        new(new("x68.filter", "Display", "Pixel filter", OptionKind.Choice, OptionValue.From("0"),
            "Changes the picture filter.",
            Choices: new[] { new ChoiceItem("0", "Sharp pixels"), new ChoiceItem("2", "Scale2x"),
                             new ChoiceItem("1", "xBR smooth"), new ChoiceItem("3", "Scanlines") }),
            "X68000", "filter"),
    };

    internal static IEnumerable<OptionDefinition> Options => Fields.Select(setting => setting.Option);

    // RetroRecomp's Windows writer can append CRLF to an existing LF file.
    internal static NativeSettings.Document ReadFile(string executable) =>
        NativeSettings.Document.Read(Path.Combine(Path.GetDirectoryName(executable)!, "datas", "Retro-Recomp.ini"),
            acceptMixedLineEndings: true);

    internal static SettingsSnapshot Read(SettingsSnapshot paths, NativeSettings.Document document)
    {
        foreach (var field in Fields)
        {
            var option = field.Option;
            var raw = document.IniValue(field.IniSection, field.Key);
            var parsed = raw != null && option.Choices!.Any(choice => choice.Value == raw)
                ? OptionValue.From(raw) : option.DefaultValue.Clone();
            paths.Values[option.Id] = parsed;
        }
        var bytes = document.Original;
        return paths with { Revision = paths.Revision + ":" + (bytes == null ? "missing" : Convert.ToHexString(SHA256.HashData(bytes))) };
    }

    internal static bool Changed(SettingsSnapshot current, IReadOnlyDictionary<string, JsonElement> values) =>
        Fields.Any(field => Different(current, values, field));

    private static bool Different(SettingsSnapshot current, IReadOnlyDictionary<string, JsonElement> values, Setting field) =>
        current.Values[field.Option.Id].GetRawText() != values[field.Option.Id].GetRawText();

    internal static void Apply(NativeSettings.Document document, SettingsSnapshot current, IReadOnlyDictionary<string, JsonElement> values)
    {
        foreach (var field in Fields)
        {
            if (!Different(current, values, field)) continue;
            var value = values[field.Option.Id];
            document.SetIni(field.IniSection, field.Key, value.GetString()!);
        }
        if (document.Changed) document.Commit();
    }
}
