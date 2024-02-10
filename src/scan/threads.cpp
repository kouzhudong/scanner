#include "threads.h"
#include "tcp.h"


LONG volatile g_stop_scan;

UINT64 g_TotalNumbers = (UINT64)((UINT64)2 << 32);//Ҫɨ���IP�������ڼ�����ȣ��ٷֱȣ���

set<DWORD> g_IPv4;
CRITICAL_SECTION g_IPv4Lock;

set<string> g_IPv6;
CRITICAL_SECTION g_IPv6Lock;

LONG64 volatile g_Send_IP;

#pragma warning(disable:6011)
#pragma warning(disable:6001)


//////////////////////////////////////////////////////////////////////////////////////////////////


template <class T>
DWORD WINAPI SendThread(_In_ T * lpParameter)
//DWORD WINAPI SendThread(_In_ LPVOID lpParameter)
{
    DWORD ret = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(lpParameter);

    for (;;) {
        if (g_stop_scan) {
            break;
        }

    }

    return ret;
}


template <class T>
DWORD WINAPI ReceiveThread(_In_ T * lpParameter)
//DWORD WINAPI ReceiveThread(_In_ LPVOID lpParameter)
{
    DWORD ret = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(lpParameter);

    for (;;) {
        if (g_stop_scan) {
            break;
        }

    }

    return ret;
}


template <class T>
DWORD WINAPI ScanThread(_In_ T * lpParameter)
//DWORD WINAPI ScanThread(_In_ LPVOID lpParameter)
/*
���ܣ�ɨ���̵߳���ڡ�

���裺
1.����MAXIMUM_WAIT_OBJECTS�ķ����̣߳����Կ��ǹ���״̬����
2.����MAXIMUM_WAIT_OBJECTS�Ľ����̣߳����Կ��ǹ���״̬����
3.����ȴ�״̬���ȴ�ɨ����ɻ����û����˳�ָ�
  �ȵȴ�ɨ���̣߳���ȴ������̣߳����м����ͣ���롣
4.����ɨ�������磺д��XML/JSON/SQLITE�ȡ�
5.ɨ��Ľ�������ڴ��У������죩������map��set�ṹ��
*/
{
    DWORD ret = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(lpParameter);

    //////////////////////////////////////////////////////////////////////////////////////////////

    //PMYDATA SendDataArray[MAXIMUM_WAIT_OBJECTS];
    DWORD   SendThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {};
    HANDLE  SendThreadArray[MAXIMUM_WAIT_OBJECTS] = {};

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        //pDataArray[i] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));
        //if (pDataArray[i] == NULL) {
        //    ExitProcess(2);
        //}

        SendThreadArray[i] = CreateThread(NULL,
                                          0,
                                          SendThread,
                                          NULL,//pDataArray[i], 
                                          0,
                                          &SendThreadIdArray[i]);
        if (SendThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

        //PMYDATA ReceiveDataArray[MAXIMUM_WAIT_OBJECTS];
    DWORD   ReceiveThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {};
    HANDLE  ReceiveThreadArray[MAXIMUM_WAIT_OBJECTS] = {};

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        //pDataArray[i] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));
        //if (pDataArray[i] == NULL) {
        //    ExitProcess(2);
        //}

        ReceiveThreadArray[i] = CreateThread(NULL,
                                             0,
                                             SendThread,
                                             NULL,//pDataArray[i], 
                                             0,
                                             &ReceiveThreadIdArray[i]);
        if (ReceiveThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, SendThreadArray, TRUE, INFINITE);

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        CloseHandle(SendThreadArray[i]);
        //if (pDataArray[i] != NULL) {
        //    HeapFree(GetProcessHeap(), 0, pDataArray[i]);
        //    pDataArray[i] = NULL;    // Ensure address is not reused.
        //}
    }

    Sleep(3000);

    WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, ReceiveThreadArray, TRUE, INFINITE);

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        CloseHandle(ReceiveThreadArray[i]);
        //if (pDataArray[i] != NULL) {
        //    HeapFree(GetProcessHeap(), 0, pDataArray[i]);
        //    pDataArray[i] = NULL;    // Ensure address is not reused.
        //}
    }

    //////////////////////////////////////////////////////////////////////////////////////////////



    //////////////////////////////////////////////////////////////////////////////////////////////

    return ret;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


DWORD WINAPI SendAllIPv4Thread(_In_ LPVOID lpParameter)
{
    PSCAN_CONTEXT pDataArray = (PSCAN_CONTEXT)lpParameter;

    for (DWORD i = 0; i < pDataArray->len; i++) {
        IN_ADDR RemoteIPv4;
        RemoteIPv4.S_un.S_addr = pDataArray->start + i;

        InterlockedIncrement64(&g_Send_IP);

        if (IsSpecialIPv4(&RemoteIPv4)) {
            continue;
        }

        if (g_stop_scan) {
            break;
        }

        SendSyn4(pDataArray->fp,
                 pDataArray->SrcMac,
                 pDataArray->DesMac,
                 &pDataArray->SourceAddress.IPv4,
                 &RemoteIPv4,
                 pDataArray->RemotePort);
    }

    return ERROR_SUCCESS;
}


void ReplyAck(struct pcap_pkthdr * header, const BYTE * pkt_data, PSCAN_CONTEXT ScanContext)
/*
�����˿�ɨ��Ļص���
*/
{
    PETHERNET_HEADER eth_hdr = (PETHERNET_HEADER)pkt_data;
    PRAW_TCP tcp4 = NULL;
    PRAW6_TCP tcp6 = NULL;
    WORD RemotePort = ScanContext->RemotePort;

    UNREFERENCED_PARAMETER(header);

    switch (ntohs(eth_hdr->Type)) {
    case ETHERNET_TYPE_IPV4:
    {
        tcp4 = (PRAW_TCP)pkt_data;

        switch (tcp4->ip_hdr.Protocol) {
        case IPPROTO_TCP:
        {
            wchar_t SrcIp[46] = {0};
            //wchar_t DesIp[46] = {0};

            InetNtop(AF_INET, &tcp4->ip_hdr.SourceAddress, SrcIp, _ARRAYSIZE(SrcIp));
            //InetNtop(AF_INET, &tcp4->ip_hdr.DestinationAddress, DesIp, _ARRAYSIZE(DesIp));

            if ((tcp4->tcp_hdr.th_flags & TH_ACK) && (tcp4->tcp_hdr.th_flags & TH_SYN)) {
                if (RemotePort == ntohs(tcp4->tcp_hdr.th_sport)) {
                    printf("%ls:%d open.\n", SrcIp, ntohs(tcp4->tcp_hdr.th_sport));

                    EnterCriticalSection(&g_IPv4Lock);
                    g_IPv4.insert(tcp4->ip_hdr.SourceAddress.S_un.S_addr);
                    LeaveCriticalSection(&g_IPv4Lock);

                    printf("�Ѿ�����%I64d����ɱ�%f, ��ȡ��%zd.\n",
                           g_Send_IP,
                           (double)g_Send_IP / g_TotalNumbers,
                           g_IPv4.size());
                }
            } else if (tcp4->tcp_hdr.th_flags & TH_RST) {
                //printf("%ls:%d close.\n", SrcIp, ntohs(tcp4->tcp_hdr.th_sport));//���͹رղ�����Ҳ�����ǿ��ġ�
            } else {
                //printf("%ls:%d unknow.\n", SrcIp, ntohs(tcp4->tcp_hdr.th_sport));
            }

            break;
        }
        default:
            break;
        }

        break;
    }
    case ETHERNET_TYPE_IPV6:
    {
        tcp6 = (PRAW6_TCP)pkt_data;

        switch (tcp6->ip_hdr.NextHeader) {
        case IPPROTO_TCP:
        {
            wchar_t SrcIp[46];
            wchar_t DesIp[46];

            InetNtop(AF_INET6, &tcp6->ip_hdr.SourceAddress, SrcIp, _ARRAYSIZE(SrcIp));
            InetNtop(AF_INET6, &tcp6->ip_hdr.DestinationAddress, DesIp, _ARRAYSIZE(DesIp));

            if ((tcp6->tcp_hdr.th_flags & TH_ACK) && (tcp6->tcp_hdr.th_flags & TH_SYN)) {
                printf("%ls:%d open.\n", SrcIp, ntohs(tcp6->tcp_hdr.th_sport));
            } else if (tcp6->tcp_hdr.th_flags & TH_RST) {
                //printf("%ls:%d close.\n", SrcIp, ntohs(tcp6->tcp_hdr.th_sport));//���͹رղ�����Ҳ�����ǿ��ġ�
            } else {
                //printf("%ls:%d unknow.\n", SrcIp, ntohs(tcp6->tcp_hdr.th_sport));
            }

            break;
        }
        default:
            break;
        }

        break;
    }
    default:
        break;
    }
}


DWORD WINAPI ReceiveThread(_In_ LPVOID lpParameter)
{
    PSCAN_CONTEXT pDataArray = (PSCAN_CONTEXT)lpParameter;

    UINT res;
    struct pcap_pkthdr * header;
    const BYTE * pkt_data;
    while ((res = pcap_next_ex(pDataArray->fp, &header, &pkt_data)) >= 0) {
        //if (g_stop_scan) {
        //    break;
        //}

        if (PCAP_ERROR_BREAK == res) {
            break;
        }

        if (res == 0) {
            continue;
        }

        if (pkt_data) {
            if (pDataArray->CallBack) {
                pDataArray->CallBack(header, pkt_data, pDataArray);
            }
        }
    }

    return ERROR_SUCCESS;
}


void OpenIPv4()
/*
���ܣ������ռ����Ŀ���ĳ���˿ڵ�IPv4��ַ��

*/
{
    size_t n = g_IPv4.size();

    printf("\n");
    printf("������%lld.\n", n);
    printf("\n");

    for (const auto & node : g_IPv4) {
        wchar_t str[46] = {0};

        IN_ADDR RemoteIPv4;
        RemoteIPv4.S_un.S_addr = node;

        InetNtop(AF_INET, &RemoteIPv4, str, _ARRAYSIZE(str));

        printf("%ls open.\n", str);
    }
}


DWORD WINAPI ScanAllIPv4Thread(_In_ LPVOID lpParameter)
/*
���ܣ�ɨ���̵߳���ڡ�

���裺
1.����MAXIMUM_WAIT_OBJECTS�ķ����̣߳����Կ��ǹ���״̬����
2.����MAXIMUM_WAIT_OBJECTS�Ľ����̣߳����Կ��ǹ���״̬����
3.����ȴ�״̬���ȴ�ɨ����ɻ����û����˳�ָ�
  �ȵȴ�ɨ���̣߳���ȴ������̣߳����м����ͣ���롣
4.����ɨ�������磺д��XML/JSON/SQLITE�ȡ�
5.ɨ��Ľ�������ڴ��У������죩������map��set�ṹ��
*/
{
    DWORD ret = ERROR_SUCCESS;
    string source;
    PSCAN_CONTEXT ScanContext = (PSCAN_CONTEXT)lpParameter;

    GetActivityAdapter(source);

    pcap_t * fp;
    int snaplen = sizeof(RAW_TCP) + sizeof(TCP_OPT_MSS);
    char errbuf[PCAP_ERRBUF_SIZE];
    if ((fp = pcap_open(source.c_str(),				// name of the device
                        snaplen,// portion of the packet to capture
                        PCAP_OPENFLAG_PROMISCUOUS, 	// promiscuous mode
                        1,				    // read timeout
                        NULL,				// authentication on the remote machine
                        errbuf				// error buffer
    )) == NULL) {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    UINT8 SrcMac[6] = {0};
    GetMacAddress(source.c_str(), SrcMac);

    IN_ADDR SourceAddress;
    GetOneAddress(source.c_str(), &SourceAddress, NULL, NULL);

    UINT8 DesMac[6] = {0};
    GetGatewayMacByIPv4(inet_ntoa(SourceAddress), DesMac);
    if (DesMac[0] == 0 && DesMac[1] == 0 && DesMac[2] == 0 &&
        DesMac[3] == 0 && DesMac[4] == 0 && DesMac[5] == 0) {
        fprintf(stderr, "û�л�ȡ��%s�����ص�������ַ��ɨ���˳�\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT SendDataArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD   SendThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {0};
    HANDLE  SendThreadArray[MAXIMUM_WAIT_OBJECTS] = {0};

    //��ͨ��1��DWORD���������з��Ż����޷��ŵģ�����������32�Ľ����0.
    DWORD step = ((DWORD64)1 << 32) / MAXIMUM_WAIT_OBJECTS;
    step = ((DWORD64)MAXDWORD + 1) / MAXIMUM_WAIT_OBJECTS;

    InitializeCriticalSection(&g_IPv4Lock);

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        SendDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        SendDataArray[i]->start = i * step;
        SendDataArray[i]->len = step;
        SendDataArray[i]->fp = fp;
        SendDataArray[i]->SourceAddress.IPv4.S_un.S_addr = SourceAddress.S_un.S_addr;
        SendDataArray[i]->RemotePort = ScanContext->RemotePort;
        CopyMemory(SendDataArray[i]->SrcMac, SrcMac, sizeof(SrcMac));
        CopyMemory(SendDataArray[i]->DesMac, DesMac, sizeof(DesMac));

        SendThreadArray[i] = CreateThread(NULL,
                                          0,
                                          SendAllIPv4Thread,
                                          SendDataArray[i],
                                          0,
                                          &SendThreadIdArray[i]);
        if (SendThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT ReceiveDataArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD   ReceiveThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {0};
    HANDLE  ReceiveThreadArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD MaxReceiveThread = min(1, MAXIMUM_WAIT_OBJECTS);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        ReceiveDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        ReceiveDataArray[i]->fp = fp;
        ReceiveDataArray[i]->RemotePort = ScanContext->RemotePort;
        ReceiveDataArray[i]->CallBack = ReplyAck;

        ReceiveThreadArray[i] = CreateThread(NULL,
                                             0,
                                             ReceiveThread,
                                             ReceiveDataArray[i],
                                             0,
                                             &ReceiveThreadIdArray[i]);
        if (ReceiveThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, SendThreadArray, TRUE, INFINITE);

    for (int i = 0; i < MAXIMUM_WAIT_OBJECTS; i++) {
        CloseHandle(SendThreadArray[i]);
        if (SendDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, SendDataArray[i]);
            SendDataArray[i] = NULL;
        }
    }

    Sleep(3000);

    pcap_breakloop(fp);

    WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, ReceiveThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        CloseHandle(ReceiveThreadArray[i]);
        if (ReceiveDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, ReceiveDataArray[i]);
            ReceiveDataArray[i] = NULL;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    OpenIPv4();

    //////////////////////////////////////////////////////////////////////////////////////////////

    return ret;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


DWORD WINAPI IPv4SubnetMaskSendThread(_In_ LPVOID lpParameter)
{
    PSCAN_CONTEXT pDataArray = (PSCAN_CONTEXT)lpParameter;

    _ASSERTE(pDataArray->mask <= 32);

    IN_ADDR IPv4;
    IPv4.S_un.S_addr = pDataArray->start;

    //////////////////////////////////////////////////////////////////////////////////////////////
    /*
    ������mask�������������롣
    ���ɵĲ���/˼·��
    1.��������������0��Ϊ1.
    2.��ȡ����
    3.��ת��Ϊ������
    */

    IN_ADDR Mask;
    Mask.S_un.S_addr = 0;

    for (char x = 0; x < (32 - pDataArray->mask); x++) {
        ULONG t = 1 << x;
        Mask.S_un.S_addr |= t;
    }

    Mask.S_un.S_addr = ~Mask.S_un.S_addr;

    Mask.S_un.S_addr = ntohl(Mask.S_un.S_addr);

    wchar_t buffer[46] = {0};
    InetNtop(AF_INET, &Mask, buffer, _ARRAYSIZE(buffer));
    printf("mask:%ls\n", buffer);

    //////////////////////////////////////////////////////////////////////////////////////////////
    //������ʼ/������ַ��

    IN_ADDR base;
    base.S_un.S_addr = IPv4.S_un.S_addr & Mask.S_un.S_addr;

    wchar_t Base[46] = {0};
    InetNtop(AF_INET, &base, Base, _ARRAYSIZE(Base));
    printf("BaseAddr:%ls\n", Base);

    //////////////////////////////////////////////////////////////////////////////////////////////
    //��ӡ��Ϣ��

    UINT64 numbers = (UINT64)1 << (32 - pDataArray->mask);//������64λ������������������

    g_TotalNumbers = numbers;

    printf("IPv4����������λ����%d\n", pDataArray->mask);
    printf("IPv4��ַ���������������ַ����%I64d\n", numbers);
    printf("\n");

    for (ULONG i = 0; i < numbers; i++) {
        IN_ADDR temp;
        temp.S_un.S_addr = base.S_un.S_addr + ntohl(i);

        //wchar_t buf[46] = {0};
        //InetNtop(AF_INET, &temp, buf, _ARRAYSIZE(buf));
        //printf("%ls\n", buf);

        InterlockedIncrement64(&g_Send_IP);

        if (g_stop_scan) {
            break;
        }

        SendSyn4(pDataArray->fp,
                 pDataArray->SrcMac,
                 pDataArray->DesMac,
                 &pDataArray->SourceAddress.IPv4,
                 &temp,
                 pDataArray->RemotePort);
    }

    return ERROR_SUCCESS;
}


DWORD WINAPI IPv4SubnetScanThread(_In_ LPVOID lpParameter)
/*
���ܣ�ɨ���̵߳���ڡ�

���裺
1.����һ�������̣߳����Կ��ǹ���״̬����
2.����һ�������̣߳����Կ��ǹ���״̬����
3.����ȴ�״̬���ȴ�ɨ����ɻ����û����˳�ָ�
  �ȵȴ�ɨ���̣߳���ȴ������̣߳����м����ͣ���롣
4.����ɨ�������磺д��XML/JSON/SQLITE�ȡ�
5.ɨ��Ľ�������ڴ��У������죩������map��set�ṹ��
*/
{
    PSCAN_CONTEXT ScanContext = (PSCAN_CONTEXT)lpParameter;
    string source;

    GetActivityAdapter(source);

    pcap_t * fp;
    int snaplen = sizeof(RAW_TCP) + sizeof(TCP_OPT_MSS);
    char errbuf[PCAP_ERRBUF_SIZE];
    if ((fp = pcap_open(source.c_str(),				// name of the device
                        snaplen,// portion of the packet to capture
                        PCAP_OPENFLAG_PROMISCUOUS, 	// promiscuous mode
                        1,				    // read timeout
                        NULL,				// authentication on the remote machine
                        errbuf				// error buffer
    )) == NULL) {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    UINT8 SrcMac[6] = {0};
    GetMacAddress(source.c_str(), SrcMac);

    IN_ADDR SourceAddress = {0};
    GetOneAddress(source.c_str(), &SourceAddress, NULL, NULL);

    UINT8 DesMac[6] = {0};
    GetGatewayMacByIPv4(inet_ntoa(SourceAddress), DesMac);
    if (DesMac[0] == 0 && DesMac[1] == 0 && DesMac[2] == 0 &&
        DesMac[3] == 0 && DesMac[4] == 0 && DesMac[5] == 0) {
        fprintf(stderr, "û�л�ȡ��%s�����ص�������ַ��ɨ���˳�\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT SendDataArray[MAXIMUM_WAIT_OBJECTS] = {};
    DWORD   SendThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {};
    HANDLE  SendThreadArray[MAXIMUM_WAIT_OBJECTS] = {};
    DWORD MaxSendThread = min(1, MAXIMUM_WAIT_OBJECTS);

    InitializeCriticalSection(&g_IPv4Lock);

    for (DWORD i = 0; i < MaxSendThread; i++) {
        SendDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        SendDataArray[i]->start = ScanContext->start;
        SendDataArray[i]->mask = ScanContext->mask;
        SendDataArray[i]->fp = fp;
        SendDataArray[i]->SourceAddress.IPv4.S_un.S_addr = SourceAddress.S_un.S_addr;
        SendDataArray[i]->RemotePort = ScanContext->RemotePort;
        CopyMemory(SendDataArray[i]->SrcMac, SrcMac, sizeof(SrcMac));
        CopyMemory(SendDataArray[i]->DesMac, DesMac, sizeof(DesMac));

        SendThreadArray[i] = CreateThread(NULL,
                                          0,
                                          IPv4SubnetMaskSendThread,
                                          SendDataArray[i],
                                          0,
                                          &SendThreadIdArray[i]);
        if (SendThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT ReceiveDataArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD   ReceiveThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {0};
    HANDLE  ReceiveThreadArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD MaxReceiveThread = min(1, MAXIMUM_WAIT_OBJECTS);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        ReceiveDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        ReceiveDataArray[i]->fp = fp;
        ReceiveDataArray[i]->RemotePort = ScanContext->RemotePort;
        ReceiveDataArray[i]->CallBack = ReplyAck;

        ReceiveThreadArray[i] = CreateThread(NULL,
                                             0,
                                             ReceiveThread,
                                             ReceiveDataArray[i],
                                             0,
                                             &ReceiveThreadIdArray[i]);
        if (ReceiveThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    WaitForMultipleObjects(MaxSendThread, SendThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxSendThread; i++) {
        CloseHandle(SendThreadArray[i]);
        if (SendDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, SendDataArray[i]);
            SendDataArray[i] = NULL;
        }
    }

    Sleep(3000);

    pcap_breakloop(fp);

    WaitForMultipleObjects(MaxReceiveThread, ReceiveThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        CloseHandle(ReceiveThreadArray[i]);
        if (ReceiveDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, ReceiveDataArray[i]);
            ReceiveDataArray[i] = NULL;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    OpenIPv4();

    //////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


void OpenPort()
/*
���ܣ������ռ����Ŀ���ĳ���˿ڵ�IPv4��ַ��

*/
{
    size_t n = g_IPv4.size();

    printf("%zd port open.\n", n);

    for (const auto & node : g_IPv4) {
        printf("port:%d open.\n", node);
    }
}


void IPv4PortReplyAck(struct pcap_pkthdr * header, const BYTE * pkt_data, _ScanContext * ScanContext)
{
    PETHERNET_HEADER eth_hdr = (PETHERNET_HEADER)pkt_data;
    PRAW_TCP tcp4 = NULL;
    ULONG S_addr = ScanContext->DestinationAddress.IPv4.S_un.S_addr;

    UNREFERENCED_PARAMETER(header);

    switch (ntohs(eth_hdr->Type)) {
    case ETHERNET_TYPE_IPV4:
    {
        tcp4 = (PRAW_TCP)pkt_data;

        switch (tcp4->ip_hdr.Protocol) {
        case IPPROTO_TCP:
        {
            wchar_t SrcIp[46] = {0};
            //wchar_t DesIp[46] = {0};

            InetNtop(AF_INET, &tcp4->ip_hdr.SourceAddress, SrcIp, _ARRAYSIZE(SrcIp));
            //InetNtop(AF_INET, &tcp4->ip_hdr.DestinationAddress, DesIp, _ARRAYSIZE(DesIp));

            if ((tcp4->tcp_hdr.th_flags & TH_ACK) && (tcp4->tcp_hdr.th_flags & TH_SYN)) {
                if (S_addr == tcp4->ip_hdr.SourceAddress.S_un.S_addr) {
                    printf("%ls:%d open.\n", SrcIp, ntohs(tcp4->tcp_hdr.th_sport));

                    EnterCriticalSection(&g_IPv4Lock);
                    g_IPv4.insert(ntohs(tcp4->tcp_hdr.th_sport));
                    LeaveCriticalSection(&g_IPv4Lock);

                    printf("�Ѿ�����%I64d����ɱ�%f, ��ȡ��%zd.\n",
                           g_Send_IP,
                           (double)g_Send_IP / g_TotalNumbers,
                           g_IPv4.size());
                }
            } else if (tcp4->tcp_hdr.th_flags & TH_RST) {
                //printf("%ls:%d close.\n", SrcIp, ntohs(tcp4->tcp_hdr.th_sport));//���͹رղ�����Ҳ�����ǿ��ġ�
            } else {
                //printf("%ls:%d unknow.\n", SrcIp, ntohs(tcp4->tcp_hdr.th_sport));
            }

            break;
        }
        default:
            break;
        }

        break;
    }
    default:
        break;
    }
}


DWORD WINAPI IPv4PortSendThread(_In_ LPVOID lpParameter)
{
    PSCAN_CONTEXT pDataArray = (PSCAN_CONTEXT)lpParameter;

    g_TotalNumbers = (UINT64)pDataArray->EndPort - (UINT64)pDataArray->StartPort + 1;

    IN_ADDR DestinationAddress;
    DestinationAddress.S_un.S_addr = pDataArray->start;

    //����DWORD������ͷ�ֱ�Ϊ0�ˣ��ٴ�ѭ����
    for (DWORD RemotePort = pDataArray->StartPort; RemotePort <= pDataArray->EndPort; RemotePort++) {

        InterlockedIncrement64(&g_Send_IP);

        if (g_stop_scan) {
            break;
        }

        SendSyn4(pDataArray->fp,
                 pDataArray->SrcMac,
                 pDataArray->DesMac,
                 &pDataArray->SourceAddress.IPv4,
                 &DestinationAddress,
                 (WORD)RemotePort);
    }

    return ERROR_SUCCESS;
}


DWORD WINAPI IPv4PortScanThread(_In_ LPVOID lpParameter)
/*
���ܣ�ɨ���̵߳���ڡ�

���裺
1.����һ�������̣߳����Կ��ǹ���״̬����
2.����һ�������̣߳����Կ��ǹ���״̬����
3.����ȴ�״̬���ȴ�ɨ����ɻ����û����˳�ָ�
  �ȵȴ�ɨ���̣߳���ȴ������̣߳����м����ͣ���롣
4.����ɨ�������磺д��XML/JSON/SQLITE�ȡ�
5.ɨ��Ľ�������ڴ��У������죩������map��set�ṹ��

��SYN���̣߳�ɨ��65535���˿ڣ����Ǻܿ�ģ�˲�䣨������10�룩��ɡ�
*/
{
    PSCAN_CONTEXT ScanContext = (PSCAN_CONTEXT)lpParameter;
    string source;

    GetActivityAdapter(source);

    pcap_t * fp;
    int snaplen = sizeof(RAW_TCP) + sizeof(TCP_OPT_MSS);
    char errbuf[PCAP_ERRBUF_SIZE];
    if ((fp = pcap_open(source.c_str(),				// name of the device
                        snaplen,// portion of the packet to capture
                        PCAP_OPENFLAG_PROMISCUOUS, 	// promiscuous mode
                        1,				    // read timeout
                        NULL,				// authentication on the remote machine
                        errbuf				// error buffer
    )) == NULL) {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    UINT8 SrcMac[6] = {0};
    GetMacAddress(source.c_str(), SrcMac);
    if (SrcMac[0] == 0 && SrcMac[1] == 0 && SrcMac[2] == 0 &&
        SrcMac[3] == 0 && SrcMac[4] == 0 && SrcMac[5] == 0) {
        fprintf(stderr, "û�л�ȡ��%s��������������ַ��ɨ���˳�\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    IN_ADDR SourceAddress = IN4ADDR_ANY_INIT;
    GetOneAddress(source.c_str(), &SourceAddress, NULL, NULL);
    if (IN4_IS_ADDR_UNSPECIFIED(&SourceAddress)) {
        fprintf(stderr, "û�л�ȡ��%s�ı�����ַ��ɨ���˳�\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    UINT8 DesMac[6] = {0};
    GetGatewayMacByIPv4(inet_ntoa(SourceAddress), DesMac);
    if (DesMac[0] == 0 && DesMac[1] == 0 && DesMac[2] == 0 &&
        DesMac[3] == 0 && DesMac[4] == 0 && DesMac[5] == 0) {
        fprintf(stderr, "û�л�ȡ��%s�����ص�������ַ��ɨ���˳�\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT SendDataArray[MAXIMUM_WAIT_OBJECTS] = {};
    DWORD   SendThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {};
    HANDLE  SendThreadArray[MAXIMUM_WAIT_OBJECTS] = {};
    DWORD MaxSendThread = min(1, MAXIMUM_WAIT_OBJECTS);

    InitializeCriticalSection(&g_IPv4Lock);

    for (DWORD i = 0; i < MaxSendThread; i++) {
        SendDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        SendDataArray[i]->start = ScanContext->start;
        SendDataArray[i]->StartPort = ScanContext->StartPort;
        SendDataArray[i]->fp = fp;
        SendDataArray[i]->SourceAddress.IPv4.S_un.S_addr = SourceAddress.S_un.S_addr;
        SendDataArray[i]->EndPort = ScanContext->EndPort;
        CopyMemory(SendDataArray[i]->SrcMac, SrcMac, sizeof(SrcMac));
        CopyMemory(SendDataArray[i]->DesMac, DesMac, sizeof(DesMac));

        SendThreadArray[i] = CreateThread(NULL,
                                          0,
                                          IPv4PortSendThread,
                                          SendDataArray[i],
                                          0,
                                          &SendThreadIdArray[i]);
        if (SendThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT ReceiveDataArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD   ReceiveThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {0};
    HANDLE  ReceiveThreadArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD MaxReceiveThread = min(1, MAXIMUM_WAIT_OBJECTS);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        ReceiveDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        ReceiveDataArray[i]->fp = fp;
        ReceiveDataArray[i]->DestinationAddress.IPv4.S_un.S_addr = ScanContext->start;
        ReceiveDataArray[i]->CallBack = IPv4PortReplyAck;

        ReceiveThreadArray[i] = CreateThread(NULL,
                                             0,
                                             ReceiveThread,
                                             ReceiveDataArray[i],
                                             0,
                                             &ReceiveThreadIdArray[i]);
        if (ReceiveThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    WaitForMultipleObjects(MaxSendThread, SendThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxSendThread; i++) {
        CloseHandle(SendThreadArray[i]);
        if (SendDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, SendDataArray[i]);
            SendDataArray[i] = NULL;
        }
    }

    Sleep(3000);

    pcap_breakloop(fp);

    WaitForMultipleObjects(MaxReceiveThread, ReceiveThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        CloseHandle(ReceiveThreadArray[i]);
        if (ReceiveDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, ReceiveDataArray[i]);
            ReceiveDataArray[i] = NULL;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    OpenPort();

    //////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


void IPv6PortReplyAck(struct pcap_pkthdr * header, const BYTE * pkt_data, _ScanContext * ScanContext)
{
    PETHERNET_HEADER eth_hdr = (PETHERNET_HEADER)pkt_data;
    PIN6_ADDR DestinationAddress = &ScanContext->DestinationAddress.IPv6;

    UNREFERENCED_PARAMETER(header);

    switch (ntohs(eth_hdr->Type)) {
    case ETHERNET_TYPE_IPV6:
    {
        PRAW6_TCP tcp6 = (PRAW6_TCP)pkt_data;

        switch (tcp6->ip_hdr.NextHeader) {
        case IPPROTO_TCP:
        {
            if ((tcp6->tcp_hdr.th_flags & TH_ACK) && (tcp6->tcp_hdr.th_flags & TH_SYN)) {
                if (IN6_ADDR_EQUAL(&tcp6->ip_hdr.SourceAddress, DestinationAddress)) {
                    wchar_t SrcIp[46];
                    wchar_t DesIp[46];

                    InetNtop(AF_INET6, &tcp6->ip_hdr.SourceAddress, SrcIp, _ARRAYSIZE(SrcIp));
                    InetNtop(AF_INET6, &tcp6->ip_hdr.DestinationAddress, DesIp, _ARRAYSIZE(DesIp));

                    printf("IPv6:%ls    Port:%d open.\n", SrcIp, ntohs(tcp6->tcp_hdr.th_sport));

                    EnterCriticalSection(&g_IPv4Lock);
                    g_IPv4.insert(ntohs(tcp6->tcp_hdr.th_sport));
                    LeaveCriticalSection(&g_IPv4Lock);

                    printf("�Ѿ�����%I64d����ɱ�%f, ��ȡ��%zd.\n",
                           g_Send_IP,
                           (double)g_Send_IP / g_TotalNumbers,
                           g_IPv4.size());
                }
            } else if (tcp6->tcp_hdr.th_flags & TH_RST) {
                //printf("%ls:%d close.\n", SrcIp, ntohs(tcp6->tcp_hdr.th_sport));//���͹رղ�����Ҳ�����ǿ��ġ�
            } else {
                //printf("%ls:%d unknow.\n", SrcIp, ntohs(tcp6->tcp_hdr.th_sport));
            }

            break;
        }
        default:
            break;
        }

        break;
    }
    default:
        break;
    }
}


DWORD WINAPI IPv6PortSendThread(_In_ LPVOID lpParameter)
{
    PSCAN_CONTEXT pDataArray = (PSCAN_CONTEXT)lpParameter;

    g_TotalNumbers = (UINT64)pDataArray->EndPort - (UINT64)pDataArray->StartPort + 1;

    //����DWORD������ͷ�ֱ�Ϊ0�ˣ��ٴ�ѭ����
    for (DWORD RemotePort = pDataArray->StartPort; RemotePort <= pDataArray->EndPort; RemotePort++) {

        InterlockedIncrement64(&g_Send_IP);

        if (g_stop_scan) {
            break;
        }

        SendSyn6(pDataArray->fp,
                 pDataArray->SrcMac,
                 pDataArray->DesMac,
                 &pDataArray->SourceAddress.IPv6,
                 &pDataArray->DestinationAddress.IPv6,
                 (WORD)RemotePort);
    }

    return ERROR_SUCCESS;
}


DWORD WINAPI IPv6PortScanThread(_In_ LPVOID lpParameter)
/*
���ܣ�ɨ���̵߳���ڡ�

���裺
1.����һ�������̣߳����Կ��ǹ���״̬����
2.����һ�������̣߳����Կ��ǹ���״̬����
3.����ȴ�״̬���ȴ�ɨ����ɻ����û����˳�ָ�
  �ȵȴ�ɨ���̣߳���ȴ������̣߳����м����ͣ���롣
4.����ɨ�������磺д��XML/JSON/SQLITE�ȡ�
5.ɨ��Ľ�������ڴ��У������죩������map��set�ṹ��

��SYN���̣߳�ɨ��65535���˿ڣ����Ǻܿ�ģ�˲�䣨������10�룩��ɡ�
*/
{
    PSCAN_CONTEXT ScanContext = (PSCAN_CONTEXT)lpParameter;

    //string source = "rpcap://\\Device\\NPF_{FFE7800C-E306-41B7-A3FE-5C6559A83F1D}";//����������ר�á�

    string source;
    GetActivityAdapter(source);//���������ԡ�

    pcap_t * fp;
    int snaplen = sizeof(RAW6_TCP);
    char errbuf[PCAP_ERRBUF_SIZE];
    if ((fp = pcap_open(source.c_str(),				// name of the device
                        snaplen,// portion of the packet to capture
                        PCAP_OPENFLAG_PROMISCUOUS, 	// promiscuous mode
                        1,				    // read timeout
                        NULL,				// authentication on the remote machine
                        errbuf				// error buffer
    )) == NULL) {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    UINT8 SrcMac[6] = {0};
    GetMacAddress(source.c_str(), SrcMac);
    if (SrcMac[0] == 0 && SrcMac[1] == 0 && SrcMac[2] == 0 &&
        SrcMac[3] == 0 && SrcMac[4] == 0 && SrcMac[5] == 0) {
        fprintf(stderr, "û�л�ȡ��%s��������������ַ��ɨ���˳�\n", source.c_str());
        return ERROR_INVALID_HANDLE;
    }

    IN6_ADDR LinkLocalIPv6Address = IN6ADDR_ANY_INIT;
    IN6_ADDR GlobalIPv6Address = IN6ADDR_ANY_INIT;
    GetOneAddress(source.c_str(), NULL, &LinkLocalIPv6Address, &GlobalIPv6Address);

    DWORD ipbufferlength = 46;
    char ipstringbuffer[46] = {0};
    inet_ntop(AF_INET6, &LinkLocalIPv6Address, ipstringbuffer, ipbufferlength);

    if (IN6_IS_ADDR_LINKLOCAL(&ScanContext->DestinationAddress.IPv6)) {
        if (IN6_IS_ADDR_UNSPECIFIED(&LinkLocalIPv6Address)) {
            fprintf(stderr, "������û�б���IPv6��ַ�����ܽ���IPv6������ɨ��\n");
            return ERROR_INVALID_HANDLE;
        }
    }

    if (IN6_IS_ADDR_GLOBAL(&ScanContext->DestinationAddress.IPv6)) {
        if (IN6_IS_ADDR_UNSPECIFIED(&GlobalIPv6Address)) {
            fprintf(stderr, "������û�л�����IPv6��ַ�����ܽ���IPv6������ɨ��\n");
            return ERROR_INVALID_HANDLE;
        }
    }

    UINT8 DesMac[6] = {0};

    if (IN6_IS_ADDR_LINKLOCAL(&ScanContext->DestinationAddress.IPv6)) {
        char RemoteIPv6[46] = {0};
        InetNtopA(AF_INET6, &ScanContext->DestinationAddress.IPv6, RemoteIPv6, _ARRAYSIZE(RemoteIPv6));

        GetMacByIPv6(RemoteIPv6, DesMac);
        if (DesMac[0] == 0 && DesMac[1] == 0 && DesMac[2] == 0 &&
            DesMac[3] == 0 && DesMac[4] == 0 && DesMac[5] == 0) {
            fprintf(stderr, "û�л�ȡ�������������ص�������ַ��ɨ���˳�\n");
            return ERROR_INVALID_HANDLE;//IPv6�Ļ������;�����ɨ����붼��Ŀ��MAC��
        }
    }

    if (IN6_IS_ADDR_GLOBAL(&ScanContext->DestinationAddress.IPv6)) {
        GetGatewayMacByIPv6(ipstringbuffer, DesMac);
        if (DesMac[0] == 0 && DesMac[1] == 0 && DesMac[2] == 0 &&
            DesMac[3] == 0 && DesMac[4] == 0 && DesMac[5] == 0) {
            fprintf(stderr, "û�л�ȡ�������������ص�������ַ��ɨ���˳�\n");
            return ERROR_INVALID_HANDLE;//IPv6�Ļ������;�����ɨ����붼��Ŀ��MAC��
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT SendDataArray[MAXIMUM_WAIT_OBJECTS] = {};
    DWORD   SendThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {};
    HANDLE  SendThreadArray[MAXIMUM_WAIT_OBJECTS] = {};
    DWORD MaxSendThread = min(1, MAXIMUM_WAIT_OBJECTS);

    InitializeCriticalSection(&g_IPv4Lock);

    for (DWORD i = 0; i < MaxSendThread; i++) {
        SendDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        SendDataArray[i]->StartPort = ScanContext->StartPort;
        SendDataArray[i]->fp = fp;
        SendDataArray[i]->EndPort = ScanContext->EndPort;
        CopyMemory(SendDataArray[i]->SrcMac, SrcMac, sizeof(SrcMac));
        CopyMemory(SendDataArray[i]->DesMac, DesMac, sizeof(DesMac));

        if (IN6_IS_ADDR_LINKLOCAL(&ScanContext->DestinationAddress.IPv6)) {
            RtlCopyMemory(&SendDataArray[i]->SourceAddress.IPv6, &LinkLocalIPv6Address, sizeof(IN6_ADDR));
        } else {
            RtlCopyMemory(&SendDataArray[i]->SourceAddress.IPv6, &GlobalIPv6Address, sizeof(IN6_ADDR));
        }

        RtlCopyMemory(&SendDataArray[i]->DestinationAddress.IPv6,
                      &ScanContext->DestinationAddress.IPv6,
                      sizeof(IN6_ADDR));

        SendThreadArray[i] = CreateThread(NULL,
                                          0,
                                          IPv6PortSendThread,
                                          SendDataArray[i],
                                          0,
                                          &SendThreadIdArray[i]);
        if (SendThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    PSCAN_CONTEXT ReceiveDataArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD   ReceiveThreadIdArray[MAXIMUM_WAIT_OBJECTS] = {0};
    HANDLE  ReceiveThreadArray[MAXIMUM_WAIT_OBJECTS] = {0};
    DWORD MaxReceiveThread = min(1, MAXIMUM_WAIT_OBJECTS);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        ReceiveDataArray[i] = (PSCAN_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SCAN_CONTEXT));
        if (SendDataArray[i] == NULL) {
            ExitProcess(2);
        }

        ReceiveDataArray[i]->fp = fp;
        RtlCopyMemory(&ReceiveDataArray[i]->DestinationAddress.IPv6,
                      &ScanContext->DestinationAddress.IPv6,
                      sizeof(IN6_ADDR));
        ReceiveDataArray[i]->CallBack = IPv6PortReplyAck;

        ReceiveThreadArray[i] = CreateThread(NULL,
                                             0,
                                             ReceiveThread,
                                             ReceiveDataArray[i],
                                             0,
                                             &ReceiveThreadIdArray[i]);
        if (ReceiveThreadArray[i] == NULL) {
            ErrorHandler((LPTSTR)TEXT("CreateThread"));
            ExitProcess(3);
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    WaitForMultipleObjects(MaxSendThread, SendThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxSendThread; i++) {
        CloseHandle(SendThreadArray[i]);
        if (SendDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, SendDataArray[i]);
            SendDataArray[i] = NULL;
        }
    }

    Sleep(3000);

    pcap_breakloop(fp);

    WaitForMultipleObjects(MaxReceiveThread, ReceiveThreadArray, TRUE, INFINITE);

    for (DWORD i = 0; i < MaxReceiveThread; i++) {
        CloseHandle(ReceiveThreadArray[i]);
        if (ReceiveDataArray[i] != NULL) {
            HeapFree(GetProcessHeap(), 0, ReceiveDataArray[i]);
            ReceiveDataArray[i] = NULL;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    OpenPort();

    //////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////