using System.Diagnostics;
using Companion.Launcher.Core;

// Explicit private inputs; no ROM fixtures or owner settings bundled.
if (args.Length != 3) throw new ArgumentException("exe rom fresh-output-directory");
var exe = Path.GetFullPath(args[0]);var rom = Path.GetFullPath(args[1]);
var output = Path.GetFullPath(args[2]);
if (Directory.Exists(output)) throw new IOException("Use a fresh test output directory.");
Directory.CreateDirectory(output);
var config = Path.Combine(output, "config.ini");
File.WriteAllText(config, "[General]\nDisableFrameDelay=1\nAutosave=0\n[Sound]\nEnableAudio=0\n");
var adapter = new GameAdapter(Path.Combine(output, "launcher-data"));
var profile = adapter.Profiles[0];
var values = adapter.Definition.Options.ToDictionary(x => x.Id, x => x.DefaultValue.Clone());
values["engine.path"] = OptionValue.From(exe);values["rom.path"] = OptionValue.From(rom);
values["config.path"] = OptionValue.From(config);
if (!adapter.CheckReady(profile, values).Ready) throw new Exception("Valid inputs rejected.");
var badRom = Path.Combine(output, "wrong-rom.dat");File.WriteAllText(badRom, "synthetic wrong input");
values["rom.path"] = OptionValue.From(badRom);
if (adapter.CheckReady(profile, values).Ready) throw new Exception("Wrong ROM accepted.");
values["rom.path"] = OptionValue.From(rom);
var plan = adapter.BuildLaunchPlan(profile, values, false);
if (plan.WorkingDirectory != Path.GetDirectoryName(exe) || plan.Arguments[0] != "--no-launcher")
    throw new Exception("Native launch contract changed.");
if (plan.Arguments.Contains("--save-root")) throw new Exception("Adapter must retain native saves.");
var start = new ProcessStartInfo(plan.Executable) {
    WorkingDirectory = plan.WorkingDirectory, UseShellExecute = false,
    RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true };
foreach (var argument in plan.Arguments) start.ArgumentList.Add(argument);
// Test isolation does not change the real adapter's save contract.
start.ArgumentList.Add("--save-root");start.ArgumentList.Add(Path.Combine(output, "test-saves"));
start.Environment["SDL_VIDEODRIVER"] = "dummy";start.Environment["SDL_AUDIODRIVER"] = "dummy";
start.Environment["SNESRECOMP_RUN_FRAMES"] = "120";start.Environment["SNESRECOMP_TIER2_CAPTURE"] = "0";
using var process = Process.Start(start) ?? throw new Exception("Process did not start.");
var stdout = process.StandardOutput.ReadToEndAsync();var stderr = process.StandardError.ReadToEndAsync();
using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(30));
await process.WaitForExitAsync(timeout.Token);
File.WriteAllText(Path.Combine(output, "stdout.log"), await stdout);
var log = await stderr;File.WriteAllText(Path.Combine(output, "stderr.log"), log);
if (process.ExitCode != 0 || !log.Contains("first frame simulated")) throw new Exception("Native startup failed.");
Console.WriteLine("PASS: clean/wrong ROM, launch arguments, original save contract, isolated native startup (120 frames).");
