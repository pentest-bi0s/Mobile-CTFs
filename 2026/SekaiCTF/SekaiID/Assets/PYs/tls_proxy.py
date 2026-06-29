#!/usr/bin/env python3
import socket
import ssl
import sys
import threading


def pipe(src, dst, tag):
    total = 0
    try:
        while data := src.recv(65536):
            if total == 0:
                print(f"  {tag} first bytes: {data[:24]!r}", flush=True)
            total += len(data)
            dst.sendall(data)
    except OSError as e:
        print(f"  {tag} error: {e}", flush=True)
    finally:
        print(f"  {tag} closed after {total} bytes", flush=True)
        # half-close: tell the peer we're done writing, but let its own
        # reader drain. Don't SHUT_RDWR both — that kills the live direction.
        try:
            dst.shutdown(socket.SHUT_WR)
        except OSError:
            pass


def handle(client, host, port):
    client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    try:
        tcp = socket.create_connection((host, port))
        tcp.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        remote = ssl.create_default_context().wrap_socket(
            tcp, server_hostname=host
        )
    except OSError as e:
        print(f"[!] connect to {host}:{port} failed: {e}")
        client.close()
        return
    print('connection in', flush=True)
    threading.Thread(target=pipe, args=(client, remote, "adb->remote"), daemon=True).start()
    pipe(remote, client, "remote->adb")


def main():
    if len(sys.argv) < 3:
        print('args: [host] [port]')
        sys.exit(1)

    host, port = sys.argv[1], int(sys.argv[2])

    listener = socket.create_server(("127.0.0.1", 5555))
    listener.settimeout(1.0)
    print(f"[*] adb connect 127.0.0.1:5555  ->  {host}:{port} (TLS)")
    while True:
        try:
            client, _ = listener.accept()
        except socket.timeout:
            continue
        threading.Thread(target=handle, args=(client, host, port), daemon=True).start()


if __name__ == "__main__":
    main()
