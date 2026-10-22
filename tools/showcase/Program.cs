using System.Diagnostics;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

record Scene(string Id,string Title,string Seed,string Shot,bool Spoiler=false) {
 public override string ToString()=>Title;
}
static class Program {
 [STAThread] static void Main(string[] args) {
  if(args.Contains("--self-check")){try{Picker.Check();}catch(Exception e){File.WriteAllText(Path.Combine(AppContext.BaseDirectory,"picker-error.txt"),e.ToString());Environment.ExitCode=1;}return;}
  ApplicationConfiguration.Initialize();
  using var single=new FileStream(Path.Combine(AppContext.BaseDirectory,"picker.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None);
  if(args.Length==2&&args[0]=="--prepare"){using var p=new Picker();p.Prepare(args[1]);return;}
  Application.Run(new Picker());
 }
}
sealed class Picker:Form {
 static readonly string Root=Path.TrimEndingDirectorySeparator(AppContext.BaseDirectory);
 static readonly string Project=Directory.GetParent(Root)!.FullName;
 static readonly string Repo=Path.Combine(Project,"LagoonSNESRecomp");
 static readonly string Build=Path.Combine(Repo,"build");
 static readonly string Session=Path.Combine(Root,"Session");
 const string ExeSha="e397544b9a01ddc434ca95cb57111aab8b947db7c0c4618ced190e9f22c6a80e";
 const string Town="485fc2bff6cdf2ca4fac8ec99e412dd049722f76bfc380a797e6b32d41af7ba3";
 const string Field="6439ddda506509623a269e93ee492cd9772f94fba74d8d36f6435cc66de38369";
 static readonly Scene[] Scenes={
  new("01","Title / new game","","Let the title settle; begin a new game manually. Intro keeps its original framing."),
  new("02","Atland — widescreen walk",Town,"Walk along the houses, turn, then cross town. Capture 10–15 seconds at normal speed."),
  new("03","Atland — dialogue",Town,"Walk to a resident and talk using normal controls. Record the portrait and typing, then move again."),
  new("04","Atland — map",Town,"Open the in-game map with L2 / Left Trigger. Show doors and your position; close it and walk."),
  new("05","Atland — settings",Town,"Open the in-game special/settings menu using your configured bindings. Show the available options without changing owner settings."),
  new("06","Elf Field — combat",Field,"Explore, engage nearby enemies and show the wider actor view. This retained fixture uses assisted progression; gameplay is live."),
  new("07","Evil Spirit — boss [SPOILER]","42a9def44affe21b919264c711e88b3c5a5bd75d481f30bec11f949541b24b9d","Optional late-game assisted fixture. Fight manually; it is not a fresh campaign or balance demonstration.",true),
  new("08","Zerah — boss [SPOILER]","30f959b791f6055b8047b2dafced7e2451f144f8e2a1eaf6b16fd686183389d7","Optional late-game assisted fixture. Capture combat and the corrected phase health bar; avoid story text.",true)
 };
 readonly ListBox list=new(){Dock=DockStyle.Fill,IntegralHeight=false};
 readonly Label details=new(){AutoSize=true,MaximumSize=new(560,0)};
 readonly Label status=new(){AutoSize=true,MaximumSize=new(560,0)};
 readonly NumericUpDown take=new(){Minimum=1,Maximum=99,Value=1};
 readonly CheckBox full=new(){Text="Fullscreen 1920×1080 / 16:9 Enhanced",Checked=true,AutoSize=true};
 Process? game;
 Scene Selected=>(Scene)list.SelectedItem!;
 string TakeName=>$"Lagoon_{Selected.Id}_Take{take.Value:00}.mp4";
 public Picker(){
  Text="Lagoon — manual recording scenes";ClientSize=new(980,570);MinimumSize=new(900,570);Font=new("Segoe UI",11);
  var layout=new TableLayoutPanel{Dock=DockStyle.Fill,Padding=new(24),ColumnCount=2};
  layout.ColumnStyles.Add(new(SizeType.Absolute,320));layout.ColumnStyles.Add(new(SizeType.Percent,100));
  list.Items.AddRange(Scenes);list.SelectedIndex=0;list.SelectedIndexChanged+=(_,_)=>RefreshScene();layout.Controls.Add(list,0,0);
  var panel=new FlowLayoutPanel{Dock=DockStyle.Fill,Padding=new(20,0,0,0),FlowDirection=FlowDirection.TopDown,WrapContents=false,AutoScroll=true};
  panel.Controls.Add(new Label{Text="Record a feature, one take at a time",AutoSize=true,Font=new("Segoe UI",18,FontStyle.Bold)});
  panel.Controls.Add(details);panel.Controls.Add(full);panel.Controls.Add(new Label{Text="Take number",AutoSize=true});panel.Controls.Add(take);
  Add(panel,"Launch / reset scene",()=>Launch(true));Add(panel,"Close staged game",CloseGame);
  Add(panel,"Copy take filename",()=>Clipboard.SetText(TakeName));
  Add(panel,"Open recordings",()=>{Directory.CreateDirectory(Path.Combine(Root,"Recordings"));Open(Path.Combine(Root,"Recordings"));});
  Add(panel,"Recording guide",()=>Open(Path.Combine(Root,"RECORDING-GUIDE.md")));
  panel.Controls.Add(status);layout.Controls.Add(panel,1,0);Controls.Add(layout);RefreshScene();
  FormClosing+=(_,_)=>CloseGame();
 }
 void RefreshScene(){if(list.SelectedItem is Scene s)details.Text=s.Shot+"\n\nScene framing needs manual visual review before recording. No automated gameplay follows the load.\n";}
 void Add(FlowLayoutPanel p,string text,Action action){var b=new Button{Text=text,Width=260,Height=34};b.Click+=(_,_)=>{try{action();}catch(Exception e){MessageBox.Show(this,e.Message,"Recording scenes");}};p.Controls.Add(b);}
 static void Open(string p)=>Process.Start(new ProcessStartInfo(p){UseShellExecute=true});
 static string Hash(string p)=>Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(p))).ToLowerInvariant();
 static void GuardDirectory(string p){Directory.CreateDirectory(p);if((File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Recording session must not contain links: "+p);}
 static long SessionBytes(string dir){long bytes=0;foreach(var p in Directory.EnumerateFileSystemEntries(dir)){var a=File.GetAttributes(p);if((a&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked session input refused: "+p);bytes+=(a&FileAttributes.Directory)!=0?SessionBytes(p):new FileInfo(p).Length;}return bytes;}
 static string SeedPath(Scene s)=>Path.Combine(Project,"validation","inputs","seeds",s.Seed+".sav");
 public static void Check(){
  var exe=Path.Combine(Build,"LagoonSNESRecomp.exe");if(Hash(exe)!=ExeSha)throw new IOException("Game differs from the qualified 0.2.5 executable. Review this tool before using a newer release.");
  foreach(var s in Scenes.Where(s=>s.Seed!=""))if(Hash(SeedPath(s))!=s.Seed)throw new IOException("Missing or changed scene fixture: "+s.Title);
  foreach(var p in new[]{Path.Combine(Build,"config.ini"),Path.Combine(Build,"keybinds.ini"),Path.Combine(Project,"Lagoon (USA).sfc")})if(!File.Exists(p))throw new FileNotFoundException(p);
  File.WriteAllText(Path.Combine(Root,"picker-self-check.json"),JsonSerializer.Serialize(new{exe_sha256=ExeSha,scenes=Scenes,status="Sources and fixture checksums verified; no visible game launched",script_eof="host_main.c:1153 returns zero scripted buttons; physical controls remain active"},new JsonSerializerOptions{WriteIndented=true}));
 }
 void CloseGame(){if(game==null)return;if(!game.HasExited){game.CloseMainWindow();if(!game.WaitForExit(2500)){game.Kill();game.WaitForExit();}}game.Dispose();game=null;status.Text="Staged game closed.";}
 public void Prepare(string id){list.SelectedIndex=Array.FindIndex(Scenes,s=>s.Id==id);if(list.SelectedIndex<0)throw new ArgumentException("Unknown scene");Launch(false);}
 void Launch(bool start){
  Check();CloseGame();
  GuardDirectory(Session);var host=Path.Combine(Session,"host");var saves=Path.Combine(Session,"saves");GuardDirectory(host);GuardDirectory(saves);
  long bytes=SessionBytes(Session);
  if(bytes>64*1024*1024)throw new IOException("Private recording session exceeded 64 MiB. Review its files before recording more.");
  // One reusable input slot. Title does not load it. Never read or copy owner saves.
  if(Selected.Seed!=""){var dst=Path.Combine(saves,"save0.sav");if(File.Exists(dst)&&(File.GetAttributes(dst)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked state refused");File.Copy(SeedPath(Selected),dst,true);}
  CopyRegular(Path.Combine(Build,"keybinds.ini"),Path.Combine(host,"keybinds.ini"));
  var catalog=Path.Combine(host,"mods","preloaded");GuardDirectory(Path.Combine(host,"mods"));GuardDirectory(catalog);
  foreach(var src in Directory.EnumerateFiles(Path.Combine(Repo,"mods","preloaded","packages"),"*",SearchOption.AllDirectories)){
   var dest=Path.Combine(catalog,"packages",Path.GetRelativePath(Path.Combine(Repo,"mods","preloaded","packages"),src));EnsureParents(dest,host);CopyRegular(src,dest);
  }
  CopyRegular(Path.Combine(Build,"mods","preloaded","state.toml"),Path.Combine(catalog,"state.toml"));
  var cfg=File.ReadAllText(Path.Combine(Build,"config.ini"));
  foreach(var kv in new Dictionary<string,string>{{"Autosave","0"},{"DisableFrameDelay","0"},{"SkipLauncher","1"},{"Fullscreen",full.Checked?"1":"0"},{"WindowSize","1920x1080"},{"NewRenderer","1"},{"VSync","1"},{"Widescreen","1"},{"WideAspect","16:9"},{"RunAhead","0"},{"EnableGamepad1","true"}})
   cfg=Regex.Replace(cfg,@"(?m)^"+kv.Key+@"\s*=.*$",kv.Key+" = "+kv.Value);
  var config=Path.Combine(Session,"config.ini");File.WriteAllText(config,cfg,new UTF8Encoding(false));
  var script=Path.Combine(Session,"scene.txt");File.WriteAllText(script,Selected.Seed==""?"wait 1\n":"wait 20\nloadstate 0\nwait 30\n");
  var psi=new ProcessStartInfo(Path.Combine(Build,"LagoonSNESRecomp.exe")){WorkingDirectory=host,UseShellExecute=false,CreateNoWindow=true};
  foreach(var key in psi.Environment.Keys.ToArray())if(key.StartsWith("SNESRECOMP_")||key.StartsWith("LAGOON_"))psi.Environment.Remove(key);
  psi.Environment.Remove("SDL_VIDEODRIVER");psi.Environment.Remove("SDL_AUDIODRIVER");
  psi.Environment["LAGOON_ACHIEVEMENTS_DIAGNOSTIC"]="1";
  psi.Environment["SNESRECOMP_MSU1"]=Path.Combine(Project,"Soundtracks","X68000","lagoon-x68000");
  foreach(var arg in new[]{"--no-launcher","--config",config,"--rom",Path.Combine(Project,"Lagoon (USA).sfc"),"--save-root","../saves","--script",script})psi.ArgumentList.Add(arg);
  File.WriteAllText(Path.Combine(Session,"launch-plan.json"),JsonSerializer.Serialize(new{scene=Selected.Id,exe=psi.FileName,cwd=host,args=psi.ArgumentList.ToArray(),environment=new Dictionary<string,string>{{"LAGOON_ACHIEVEMENTS_DIAGNOSTIC","1"},{"SNESRECOMP_MSU1",psi.Environment["SNESRECOMP_MSU1"]!}},manual_input=true,normal_speed=true,visual_review="pending"},new JsonSerializerOptions{WriteIndented=true}));
  if(!start)return;
  game=Process.Start(psi)!;status.Text=$"Scene loaded in the private session. Record manually in OBS.\nSave your take as {TakeName}\nStop OBS before resetting. Normal saves and achievement progress are separate.";
 }
 static void CopyRegular(string from,string to){if(File.Exists(to)&&(File.GetAttributes(to)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked destination refused: "+to);File.Copy(from,to,true);}
 static void EnsureParents(string file,string stop){var d=Path.GetDirectoryName(file)!;if(d!=stop)EnsureParents(d,stop);GuardDirectory(d);}
}
