using System.Diagnostics;

namespace SelectRegionForDbd.Core;

public static class DnsUtil
{
    public static void Flush()
    {
        try
        {
            using var process = Process.Start(new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = "/C ipconfig /flushdns",
                CreateNoWindow = true,
                UseShellExecute = false,
                RedirectStandardOutput = true
            });
            process?.WaitForExit();
        }
        catch
        {
            // DNS flush is best-effort; ignore failures.
        }
    }
}