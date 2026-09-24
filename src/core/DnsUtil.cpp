#include "DnsUtil.h"

#include "Util.h"

namespace DnsUtil {

void Flush()
{
    util::runHidden("cmd.exe /C ipconfig /flushdns");
}

} // namespace DnsUtil