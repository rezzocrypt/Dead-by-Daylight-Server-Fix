using System.Text.Json;

namespace SelectRegionForDbd.Core;

public static class RegionCatalog
{
    private const string FileName = "regions.json";

    private static readonly JsonSerializerOptions JsonOptions = new() { PropertyNameCaseInsensitive = true };

    private static readonly Lazy<IReadOnlyList<GameliftRegion>> Loaded = new(Load);

    public static IReadOnlyList<GameliftRegion> All => Loaded.Value;

    public static IReadOnlyList<GameliftRegion> HostsOrder => All;

    public static GameliftRegion? Find(string id) => All.FirstOrDefault(r => r.Id == id);

    private static IReadOnlyList<GameliftRegion> Load()
    {
        try
        {
            string path = Path.Combine(AppContext.BaseDirectory, FileName);
            if (!File.Exists(path))
            {
                return [];
            }

            var entries = JsonSerializer.Deserialize<List<RegionEntry>>(File.ReadAllText(path), JsonOptions);
            return entries?
                .Where(e => !string.IsNullOrWhiteSpace(e.Id))
                .Select(ToRegion)
                .ToList() ?? [];
        }
        catch
        {
            return [];
        }
    }

    private static GameliftRegion ToRegion(RegionEntry entry)
    {
        string id = entry.Id!.Trim();
        string host = string.IsNullOrWhiteSpace(entry.Host)
            ? $"gamelift.{id}.amazonaws.com"
            : entry.Host.Trim();
        return new GameliftRegion
        {
            Id = id,
            Name = string.IsNullOrWhiteSpace(entry.Name) ? id : entry.Name.Trim(),
            Host = host,
            HostsGroup = string.IsNullOrWhiteSpace(entry.HostsGroup) ? "asia" : entry.HostsGroup.Trim()
        };
    }

    private sealed record RegionEntry(
        string? Id,
        string? Name,
        string? Host,
        string? HostsGroup);
}