using System.Text.Json;

namespace SelectRegionForDbd.Core;

public static class FirewallService
{
    private const string IpRangesUrl = "https://ip-ranges.amazonaws.com/ip-ranges.json";
    private const string RulePrefix = "DbdBlockRule";

    private static readonly HttpClient Http = new() { Timeout = TimeSpan.FromSeconds(30) };

    private enum Direction { Inbound, Outbound }

    private static string RuleName(string platform, string regionId, Direction direction)
    {
        string suffix = direction == Direction.Inbound ? "IN" : "OUT";
        return $"{RulePrefix}{platform}_{regionId}_{suffix}";
    }

    private static string RulePattern(string platform) => $"{RulePrefix}{platform}*";

    public static async Task<OpResult> CreateRulesAsync(GameliftRegion region, string exePath, string platform, CancellationToken cancellationToken = default)
    {
        try
        {
            var ips = await FetchBlockedIpPrefixesAsync(region, cancellationToken);
            if (ips.Count == 0)
            {
                return OpResult.Fail("No IP ranges could be collected for the selected region set.");
            }

            string ruleIn = BuildRule(ips, exePath, platform, region.Id, Direction.Inbound);
            string ruleOut = BuildRule(ips, exePath, platform, region.Id, Direction.Outbound);
            return await RunRuleScriptsAsync(ruleIn, ruleOut, cancellationToken);
        }
        catch (HttpRequestException ex)
        {
            return OpResult.Fail($"Error while requesting data: {ex.Message}");
        }
        catch (OperationCanceledException)
        {
            return OpResult.Fail("The request timed out.");
        }
        catch (Exception ex)
        {
            return OpResult.Fail($"An error occurred: {ex.Message}");
        }
    }

    public static async Task<OpResult> RemoveRulesAsync(string platform, CancellationToken cancellationToken = default)
    {
        await PowerShellRunner.RunCommandAsync(
            $"Remove-NetFirewallRule -DisplayName \"{RulePattern(platform)}\"", cancellationToken);

        bool anyRemain = await RulePatternExistsAsync(platform, cancellationToken);
        return anyRemain
            ? OpResult.Fail("Rules not found or could not be removed.")
            : OpResult.Ok;
    }

    public static async Task<(bool Found, string? RegionId)> GetAppliedRuleAsync(string platform, CancellationToken cancellationToken = default)
    {
        var result = await PowerShellRunner.RunCommandAsync(
            $"Get-NetFirewallRule -DisplayName \"{RulePattern(platform)}\" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty DisplayName",
            cancellationToken);
        if (!result.IsSuccess)
        {
            return (false, null);
        }

        string? regionId = result.Output
            .Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Select(line => ParseRegionId(line, platform))
            .FirstOrDefault(id => id is not null && RegionCatalog.Find(id) is not null);
        bool found = regionId is not null || !string.IsNullOrWhiteSpace(result.Output);
        return (found, regionId);
    }

    private static async Task<List<string>> FetchBlockedIpPrefixesAsync(GameliftRegion selected, CancellationToken cancellationToken)
    {
        string json = await Http.GetStringAsync(IpRangesUrl, cancellationToken);
        using var document = JsonDocument.Parse(json);

        var blockIds = RegionCatalog.All
            .Where(r => r.Id != selected.Id)
            .Select(r => r.Id)
            .ToHashSet();

        var ips = new List<string>();
        foreach (var entry in document.RootElement.GetProperty("prefixes").EnumerateArray())
        {
            string? region = entry.GetProperty("region").GetString();
            if (region is null || !blockIds.Contains(region))
            {
                continue;
            }

            if (entry.TryGetProperty("ip_prefix", out var prefix) && prefix.GetString() is { } ip && ip.Contains('.'))
            {
                ips.Add(ip);
            }
        }
        return ips;
    }

    private static string BuildRule(IReadOnlyList<string> ips, string exePath, string platform, string regionId, Direction direction)
    {
        string ruleName = RuleName(platform, regionId, direction);
        string remoteAddress = string.Join(",", ips);
        return $"New-NetFirewallRule -Name \"{ruleName}\" -DisplayName \"{ruleName}\" " +
               $"-Direction {direction} -Action Block -Program \"{exePath}\" -RemoteAddress {remoteAddress}";
    }

    private static async Task<OpResult> RunRuleScriptsAsync(string ruleIn, string ruleOut, CancellationToken cancellationToken)
    {
        string scriptIn = WriteTempScript(ruleIn);
        string scriptOut = WriteTempScript(ruleOut);
        try
        {
            var resultIn = await PowerShellRunner.RunScriptAsync(scriptIn, cancellationToken);
            var resultOut = await PowerShellRunner.RunScriptAsync(scriptOut, cancellationToken);
            if (resultIn.IsSuccess && resultOut.IsSuccess)
            {
                return OpResult.Ok;
            }

            string details = $"{resultIn.Error} {resultOut.Error}".Trim();
            return OpResult.Fail(string.IsNullOrWhiteSpace(details) ? "Failed to add rules." : details);
        }
        finally
        {
            File.Delete(scriptIn);
            File.Delete(scriptOut);
        }
    }

    private static async Task<bool> RulePatternExistsAsync(string platform, CancellationToken cancellationToken)
    {
        var result = await PowerShellRunner.RunCommandAsync(
            $"Get-NetFirewallRule -DisplayName \"{RulePattern(platform)}\" -ErrorAction SilentlyContinue", cancellationToken);
        return result.IsSuccess && result.Output.Contains(RulePrefix, StringComparison.OrdinalIgnoreCase);
    }

    private static string? ParseRegionId(string ruleName, string platform)
    {
        string prefix = $"{RulePrefix}{platform}_";
        if (!ruleName.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        string rest = ruleName[prefix.Length..];
        const string inSuffix = "_IN";
        const string outSuffix = "_OUT";
        if (rest.EndsWith(inSuffix, StringComparison.OrdinalIgnoreCase))
        {
            rest = rest[..^inSuffix.Length];
        }
        else if (rest.EndsWith(outSuffix, StringComparison.OrdinalIgnoreCase))
        {
            rest = rest[..^outSuffix.Length];
        }
        return rest.Length == 0 ? null : rest;
    }

    private static string WriteTempScript(string content)
    {
        string path = Path.Combine(Path.GetTempPath(), $"dbd_{Guid.NewGuid():N}.ps1");
        File.WriteAllText(path, content);
        return path;
    }
}