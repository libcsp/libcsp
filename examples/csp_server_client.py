"""
Usage: LD_LIBRARY_PATH=build PYTHONPATH=build python3 ./examples/csp_server_client.py
"""
import time
import threading
import libcsp_py3 as csp
from typing import Any, Callable


def printer(node: str, color: str) -> Callable:
    def f(inp: str) -> None:
        print('{color}[{name}]: {inp}\033[0m'.format(
            color=color, name=node.upper(), inp=inp))

    return f


def server_task(addr: int, port: int) -> None:
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
            if csp.conn_dport(conn) == port:
                _print('Recieved on {port}: {data}'.format(
                    port=port,
                    data=csp.packet_get_data(packet).decode('utf-8'))
                )
            else:
                csp.service_handler(conn, packet)


def client_task(addr: int, port: int) -> None:
    _print = printer('client', '\033[92m')
    _print('Starting client task')

    count = ord('A')

    while 1:
        time.sleep(1)

        ping = csp.ping(addr, 1000, 100, csp.CSP_O_NONE)
        _print('Ping {addr}: {ping}ms'.format(addr=addr, ping=ping))

        conn = csp.connect(csp.CSP_PRIO_NORM, addr, port, 1000, csp.CSP_O_NONE)
        if conn is None:
            raise Exception('Connection failed')

        packet = csp.buffer_get(0)
        if packet is None:
            raise Exception('Failed to get CSP buffer')

        data = bytes('Hello World {}'.format(chr(count)), 'ascii') + b'\x00'
        count += 1

        csp.packet_set_data(packet, data)
        csp.send(conn, packet)


def main() -> None:
    csp.init("", "", "")
    csp.route_start_task()

    serv_addr = 0
    serv_port = 10

    for task in (server_task, client_task):
        t = threading.Thread(target=task, args=(serv_addr, serv_port))
        t.start()

    print('Server and client started')


if __name__ == '__main__':
    main()
