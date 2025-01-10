# C-NetPool
A simple cross-platform event-based C network library that hides work with sockets and monitors the status of open connections.

Features:
 - connections always dies, even if physical cable is broken
 - ability to delay connection via timeout
 - IPv6 supported


Code notes:
 - dont destroy NetPool while dispatching it, interrupt it by NetPoolEmit
 - not thread-safe, use interruptions by NetPoolEmit and external synchronization
 - NetUnit is node in list and on event NET_CANREAD it moves after server unit (only for incoming connection)


Link with flags:
 - Windows: -lws2_32
 - Solaris: -lsocket


Look for usage examples in main.c