using System.Diagnostics;

namespace SelectRegionForDbd.Core;

public sealed record PsResult(int ExitCode, string Output, string Error)
{
    public bool IsSuccess => ExitCode == 0 && string.IsNullOrWhiteSpace(Error);
}

public static class PowerShellRunner
{
    public static Task<PsResult> RunCommandAsync(string command, CancellationToken cancellationToken = default) =>
        RunAsync(new ProcessStartInfo
        {
            FileName = "powershell.exe",
            Arguments = $"-NoProfile -ExecutionPolicy Bypass -Command \"{command}\"",
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        }, cancellationToken);

    public static Task<PsResult> RunScriptAsync(string scriptPath, CancellationToken cancellationToken = default) =>
        RunAsync(new ProcessStartInfo
        {
            FileName = "powershell.exe",
            Arguments = $"-NoProfile -ExecutionPolicy Bypass -File \"{scriptPath}\"",
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        }, cancellationToken);

    private static async Task<PsResult> RunAsync(ProcessStartInfo startInfo, CancellationToken cancellationToken)
    {
        using var process = Process.Start(startInfo);
        if (process is null)
        {
            return new PsResult(-1, string.Empty, "Failed to start powershell.exe");
        }

        Task<string> outputTask = process.StandardOutput.ReadToEndAsync(cancellationToken);
        Task<string> errorTask = process.StandardError.ReadToEndAsync(cancellationToken);
        await process.WaitForExitAsync(cancellationToken);

        string output = await outputTask;
        string error = await errorTask;
        return new PsResult(process.ExitCode, output, error);
    }
}