using System.Text;

namespace SelectRegionForDbd.Core;

public static class HostsFileService
{
    private const string HostsPath = @"C:\Windows\System32\drivers\etc\hosts";

    private static readonly string[] SectionHeaders = ["#asia", "#america", "#europe"];

    public static OpResult Apply(GameliftRegion selected)
    {
        try
        {
            var lines = File.Exists(HostsPath)
                ? File.ReadAllLines(HostsPath).Where(line => !IsOwnedLine(line)).ToList()
                : [];

            lines.AddRange(BuildBlock(selected));
            File.WriteAllLines(HostsPath, lines);
            DnsUtil.Flush();
            return OpResult.Ok;
        }
        catch (UnauthorizedAccessException)
        {
            return OpResult.Fail("You do not have permission to modify the hosts file. Run the application as administrator.");
        }
        catch (Exception ex)
        {
            return OpResult.Fail($"An error occurred: {ex.Message}");
        }
    }

    public static OpResult Clear()
    {
        try
        {
            if (!File.Exists(HostsPath))
            {
                return OpResult.Ok;
            }

            var lines = File.ReadAllLines(HostsPath).Where(line => !IsOwnedLine(line)).ToArray();
            File.WriteAllLines(HostsPath, lines);
            DnsUtil.Flush();
            return OpResult.Ok;
        }
        catch (UnauthorizedAccessException)
        {
            return OpResult.Fail("You do not have permission to modify the hosts file. Run the application as administrator.");
        }
        catch (Exception ex)
        {
            return OpResult.Fail($"An error occurred: {ex.Message}");
        }
    }

    // Применённая зона определяется по закомментированной (разрешённой) строке в hosts
    public static string? GetAppliedRegionId()
    {
        try
        {
            if (!File.Exists(HostsPath))
            {
                return null;
            }

            var commentedLines = File.ReadAllLines(HostsPath)
                .Select(line => line.Trim())
                .Where(line => line.StartsWith('#'))
                .ToArray();

            foreach (var region in RegionCatalog.All)
            {
                if (commentedLines.Any(line => line.Contains(region.Host, StringComparison.OrdinalIgnoreCase)))
                {
                    return region.Id;
                }
            }
            return null;
        }
        catch
        {
            return null;
        }
    }

    private static bool IsOwnedLine(string line)
    {
        string trimmed = line.Trim();
        return trimmed is "#asia" or "#america" or "#europe" ||
               (trimmed.Contains("gamelift.", StringComparison.OrdinalIgnoreCase) &&
                trimmed.Contains("amazonaws.com", StringComparison.OrdinalIgnoreCase));
    }

    private static IEnumerable<string> BuildBlock(GameliftRegion selected)
    {
        var block = new List<string>();
        foreach (string group in SectionHeaders)
        {
            block.Add(group);
            foreach (var region in RegionCatalog.HostsOrder.Where(r => r.HostsGroup == group))
            {
                bool commented = region.Id == selected.Id;
                string prefix = commented ? "#0.0.0.0" : "0.0.0.0";
                block.Add($"{prefix} {region.Host} # {region.Name} {region.Id}");
            }
        }
        return block;
    }
}