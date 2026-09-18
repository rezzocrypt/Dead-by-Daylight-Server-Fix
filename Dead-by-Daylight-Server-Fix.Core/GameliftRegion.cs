namespace SelectRegionForDbd.Core;

public sealed record GameliftRegion
{
    public required string Id { get; init; }
    public required string Name { get; init; }
    public required string Host { get; init; }
    public required string HostsGroup { get; init; }

    public override string ToString() => Id;
}