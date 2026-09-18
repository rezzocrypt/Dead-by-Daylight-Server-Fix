namespace SelectRegionForDbd.Core;

public sealed record OpResult(bool Success, string? Message = null)
{
    public static OpResult Ok { get; } = new(true);

    public static OpResult Fail(string message) => new(false, message);
}