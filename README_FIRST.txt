COMPUTER NETWORKS PRACTICAL EXAM - BEGINNER CODE PACK
=====================================================

IMPORTANT
---------
1. This pack covers the uploaded assignment themes:
   - NS-3 wired TCP + UDP performance simulation
   - NS-3 UDP latency / two UDP ports
   - NS-3 wireless UDP example with AODV
   - C TCP client/server
   - C UDP DNS simulation with cache, timeout and NXDOMAIN
   - C checksum (8-bit words, one's complement)
   - C CRC over TCP with manual error injection
   - C HTTP downloader for a LOCAL Python HTTP server
   - Wireshark commands and metric formulas
2. ARP is intentionally not a practical topic here, per your teacher's instruction.
3. No one can guarantee every file compiles on every NS-3 release. The NS-3 API differs by version.
   Start by checking the installed version and compile the smallest file first.
4. Your teacher's uploaded Assignment 10 mentions RIP, OSPF and AODV. Stock NS-3 commonly
   supports AODV but does not provide drop-in RIP/OSPF implementations in the standard modules.
   Do not claim to have implemented RIP/OSPF with AODV code. Ask which routing modules/libraries
   your lab has installed if this question is assigned.
5. The uploaded TCP congestion assignment asks for Reno, Cubic and BBR. Reno/Cubic/BBR availability
   depends on the NS-3 release and TCP implementation. Do not assume BBR exists in your version.

A. UBUNTU / LINUX: CHECK YOUR TOOLS
-----------------------------------
gcc --version
python3 --version
wireshark --version
# From your NS-3 directory:
./ns3 --version
# Older NS-3 may use:
./waf --version

B. COMPILE A C FILE
-------------------
gcc tcp_server.c -o tcp_server -Wall -Wextra
gcc tcp_client.c -o tcp_client -Wall -Wextra
Run server in Terminal 1:
./tcp_server
Run client in Terminal 2:
./tcp_client

For UDP DNS:
gcc udp_dns_server.c -o udp_dns_server -Wall -Wextra
gcc udp_dns_client.c -o udp_dns_client -Wall -Wextra
Run server first, then client in another terminal.

For checksum:
gcc checksum.c -o checksum -Wall -Wextra
./checksum

For CRC over TCP:
gcc crc_tcp_server.c -o crc_tcp_server -Wall -Wextra
gcc crc_tcp_client.c -o crc_tcp_client -Wall -Wextra
Terminal 1: ./crc_tcp_server
Terminal 2: ./crc_tcp_client
The client intentionally flips one bit before sending so you can see CRC error detection.
To test no error, change INJECT_ERROR to 0 in crc_tcp_client.c and recompile.

C. HTTP CLIENT PRACTICAL
------------------------
Make a server directory and run a Python server:
mkdir -p CN_Web_Server
cd CN_Web_Server
echo '<h1>Computer Networks Lab</h1>' > index.html
echo 'sample notes' > notes.txt
python3 -m http.server 8000

Keep that terminal running. In another terminal, compile and run http_client.c:
gcc http_client.c -o http_client -Wall -Wextra
./http_client
Enter: http://127.0.0.1:8000/index.html
It saves the body as downloaded.bin. This is intentionally for HTTP (not HTTPS).
Use localhost resources in the exam if internet access is unavailable.

D. NS-3 FILES
-------------
Copy the .cc files into your NS-3 scratch directory.

Modern NS-3:
./ns3 build
./ns3 run "scratch/cn_wired --bw=2"
./ns3 run "scratch/cn_udp_latency --delay=2 --flows=1"
./ns3 run "scratch/cn_wireless_aodv"

Older NS-3:
./waf build
./waf --run "scratch/cn_wired --bw=2"

Run bandwidth experiments:
./ns3 run "scratch/cn_wired --bw=2"
./ns3 run "scratch/cn_wired --bw=5"
./ns3 run "scratch/cn_wired --bw=10"

Run latency experiments:
./ns3 run "scratch/cn_udp_latency --delay=2 --flows=1"
./ns3 run "scratch/cn_udp_latency --delay=5 --flows=1"
./ns3 run "scratch/cn_udp_latency --delay=10 --flows=1"
./ns3 run "scratch/cn_udp_latency --delay=20 --flows=1"
Repeat with --flows=2.

NOTE: NS-3's command-line launcher may require a program name without ".cc".
If a command fails, inspect the error carefully. Do not switch the entire NS-3 version during a timed exam.

E. WIRESHARK
------------
Capture on the interface actually carrying your traffic (loopback for localhost traffic).
Useful display filters:
tcp
udp
http
dns
tcp.port == 5000
udp.port == 5000
Do not study/use ARP for this exam according to the teacher's message.

F. METRICS TO MEMORIZE
----------------------
PDR (%) = received packets / transmitted packets * 100
Packet loss (%) = (transmitted - received) / transmitted * 100
Throughput (Mbps) = received bytes * 8 / elapsed seconds / 1,000,000
Average delay = sum of packet delays / received packets
RTT = time reply arrives - time request was sent
For HTTP: download throughput = body bytes * 8 / download seconds / 1,000,000

Do not invent measured results. Run the program and record your own output.
