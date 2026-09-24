#pragma once

// DNS cache flush (mirror of DnsUtil.cs).
namespace DnsUtil {

// Runs "ipconfig /flushdns" in a hidden window. Best-effort, ignores failures.
void Flush();

} // namespace DnsUtil