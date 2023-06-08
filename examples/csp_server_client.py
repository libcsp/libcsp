
import time
import threading
import libcsp_py3 as csp
from typing import Any, Callable

def printer(node: str, color: str) -> Callable:
    def f(inp: str) -> None:
        print(f'{color}[{node.upper()}]: {inp}\033[0m')

    return f

def server_task(*args: Any) -> None:
    _print = printer('server', '\033[96m')
    _print('Starting server task')

    sock = csp.socket()
    csp.bind(sock, csp.CSP_ANY)

    csp.listen(sock, 10)

    while 1:
        conn = csp.accept(sock, 10000)

        if conn is None:
            continue

        while (packet := csp.read(conn, 50)) is not None:
            if csp.conn_dport(conn) == 10:
                _print(f'Recieved on 10: {csp.packet_get_data(packet).decode("utf-8")}')
                csp.buffer_free(packet)
            else:
                csp.service_handler(conn, packet)

        csp.close(conn)

def client_task(addr: int) -> None:
    _print = printer('client', '\033[92m')
    _print('Starting client task')

    count = ord('A')

    while 1:
        time.sleep(1)

        ping = csp.ping(addr, 1000, 100, csp.CSP_O_NONE)
        _print(f'Ping {addr}: {ping}ms')

        conn = csp.connect(csp.CSP_PRIO_NORM, addr, 10, 1000, csp.CSP_O_NONE)
        if conn is None:
            raise Exception('Connection failed')
        
        packet = csp.buffer_get(100)
        if packet is None:
            raise Exception('Failed to get CSP buffer')
        
        data = bytearray(f'Hello World {chr(count)}', 'ascii') + b'\x00'
        count += 1

        csp.packet_set_data(packet, data)
        csp.send(conn, packet)
        csp.close(conn)

def main() -> None:
    csp.init("", "", "")
    csp.route_start_task()

    serv_addr = 0

    for task in (server_task, client_task):
        t = threading.Thread(target=task, args=(serv_addr,))
        t.start()

    print('Server and client started')

if __name__ == '__main__':
    import sys
    import argparse

    parser = argparse.ArgumentParser()
    opts = parser.parse_args(sys.argv[1:])

    main()
