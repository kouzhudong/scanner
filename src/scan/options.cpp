#include "options.h"
#include "tcp.h"
#include "udp.h"
#include "threads.h"


//////////////////////////////////////////////////////////////////////////////////////////////////


DWORD WINAPI ScanAllIPv4(_In_ PCWSTR RemotePort)
{
    int ret = ERROR_SUCCESS;
    SCAN_CONTEXT ScanContext = {0};

    int port = _wtoi(RemotePort);
    if (0 == port) {
        return ERROR_INVALID_PARAMETER;
    }

    if (port < 0) {
        return ERROR_INVALID_PARAMETER;
    }

    if (port > MAXWORD) {
        return ERROR_INVALID_PARAMETER;
    }

    string FileName;
    FileName = g_ExePath;
    FileName += "scan.db";
    ScanContext.FileName = &FileName;

    ScanContext.RemotePort = (WORD)port;
    ret = ScanAllIPv4Thread((LPVOID)&ScanContext);

    return ret;
}


DWORD WINAPI IPv4SubnetScan(_In_ PCWSTR IPv4Subnet, _In_ PCWSTR RemotePort)
{
    int ret = ERROR_SUCCESS;
    SCAN_CONTEXT ScanContext = {0};
    //int IPv4SubnetMasks = 0;
    WCHAR IPv4[17] = {0};
    WCHAR SubnetMast[MAX_PORT_STRING_LENGTH] = {0};
    BYTE mask = 32;

    wregex ipv4((LR"((.+)/(\d{1,5}))"));
    wcmatch match;
    if (regex_search(IPv4Subnet, match, ipv4)) {
        RtlCopyMemory(IPv4, match[1].first, match[1].length() * sizeof(wchar_t));
        RtlCopyMemory(SubnetMast, match[2].first, match[2].length() * sizeof(wchar_t));

        wregex reg(IPV4_REGULARW);
        if (!regex_search(IPv4, match, reg)) {
            return ERROR_INVALID_PARAMETER;
        }
    } else {
        return ERROR_INVALID_PARAMETER;
    }

    IN_ADDR RemoteIPv4 = {0};
    if (0 == InetPton(AF_INET, IPv4, &RemoteIPv4)) {
        return ERROR_INVALID_PARAMETER;
    }

    mask = (BYTE)_wtoi(SubnetMast);//格式已经警告正则校验，这里不会出错。

    if (mask < 0) {
        return ERROR_INVALID_PARAMETER;
    }

    if (mask > 32) {
        return ERROR_INVALID_PARAMETER;
    }

    int port = _wtoi(RemotePort);
    if (0 == port) {
        return ERROR_INVALID_PARAMETER;
    }

    if (port < 0) {
        return ERROR_INVALID_PARAMETER;
    }

    if (port > MAXWORD) {
        return ERROR_INVALID_PARAMETER;
    }

    string FileName;
    FileName = g_ExePath;
    FileName += "scan.db";
    ScanContext.FileName = &FileName;

    ScanContext.RemotePort = (WORD)port;
    ScanContext.mask = mask;
    ScanContext.start = RemoteIPv4.S_un.S_addr;
    ret = IPv4SubnetScanThread((LPVOID)&ScanContext);

    return ret;
}


DWORD WINAPI SynPortScan(_In_ PCWSTR IP, _In_ PCWSTR RemotePort)
{
    int ret = ERROR_SUCCESS;
    SCAN_CONTEXT ScanContext = {0};

    IN_ADDR RemoteIPv4 = {0};
    IN6_ADDR RemoteIPv6 = {0};

    if (InetPton(AF_INET, IP, &RemoteIPv4)) {

    } else if (InetPton(AF_INET6, IP, &RemoteIPv6)) {

    } else {
        return ERROR_INVALID_PARAMETER;
    }

    WCHAR StartPortString[MAX_PORT_STRING_LENGTH] = {0};
    WCHAR EndPortString[MAX_PORT_STRING_LENGTH] = {0};

    wregex ipv4((LR"((\d{1,5})-(\d{1,5}))"));
    wcmatch match;
    if (regex_search(RemotePort, match, ipv4)) {
        RtlCopyMemory(StartPortString, match[1].first, match[1].length() * sizeof(wchar_t));
        RtlCopyMemory(EndPortString, match[2].first, match[2].length() * sizeof(wchar_t));
    } else {
        return ERROR_INVALID_PARAMETER;
    }

    int StartPort = _wtoi(StartPortString);//格式已经警告正则校验，这里不会出错。

    if (StartPort < 0) {
        return ERROR_INVALID_PARAMETER;
    }

    if (StartPort > MAXWORD) {
        return ERROR_INVALID_PARAMETER;
    }

    int EndPort = _wtoi(EndPortString);//格式已经警告正则校验，这里不会出错。

    if (EndPort < 0) {
        return ERROR_INVALID_PARAMETER;
    }

    if (EndPort > MAXWORD) {
        return ERROR_INVALID_PARAMETER;
    }

    if (StartPort > EndPort) {
        return ERROR_INVALID_PARAMETER;
    }

    ScanContext.StartPort = (WORD)StartPort;
    ScanContext.EndPort = (WORD)EndPort;

    if (InetPton(AF_INET, IP, &RemoteIPv4)) {
        ScanContext.start = RemoteIPv4.S_un.S_addr;
        ret = IPv4PortScanThread((LPVOID)&ScanContext);
    } else if (InetPton(AF_INET6, IP, &RemoteIPv6)) {
        RtlCopyMemory(&ScanContext.DestinationAddress.IPv6, &RemoteIPv6, sizeof(IN6_ADDR));
        ret = IPv6PortScanThread((LPVOID)&ScanContext);
    }

    return ret;
}


void test()
{
    string source;
    GetActivityAdapter(source);

    //Syn4ScanTest(source.c_str(), "58.30.226.47", 3389);
    //Syn6ScanTest(source.c_str(), "fe80::95c9:6378:91c0:d5b2", 80);//2001:4860:4860::6464 53
    test_udp4(source.c_str());
}


int ParseCommandLine(_In_ int argc, _In_reads_(argc) WCHAR * argv[])
{
    int ret = ERROR_SUCCESS;

    switch (argc) {
    case 3:
        if (lstrcmpi(argv[1], L"SYN") == 0) {
            ret = ScanAllIPv4(argv[2]);
        } else {
            //Usage(argv[0]);
        }
        break;
    case 4:
        if (lstrcmpi(argv[1], L"SYN") == 0) {
            wregex ipv4(LR"((\d{1,5})-(\d{1,5}))");
            wcmatch match;
            if (regex_search(argv[3], match, ipv4)) {
                ret = SynPortScan(argv[2], argv[3]);
            } else {
                ret = IPv4SubnetScan(argv[2], argv[3]);
            }
        } else {
            //Usage(argv[0]);
        }
        break;
    default:
        //Usage(argv[0]);
        break;
    }    

    return ret;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
