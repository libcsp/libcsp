use std::net::{TcpListener, TcpStream};
use std::io::{Read, Write};
use std::str;

fn main() {
    // TcpListener implements a server
    let listener = TcpListener::bind("127.0.0.1:8080").unwrap(); println!("Server listening on port 8080");

    for stream in listener.incoming() {
        let mut stream = stream.unwrap();
        println!("Client connected");

        let response = "Hello from Rust!\n";
        stream.write(response.as_bytes()).unwrap();
        println!("Message sent to client");

        let mut buffer: [u8; 512] = [0; 512];
        stream.read(&mut buffer).unwrap();
        let s = match str::from_utf8(&buffer) {
            Ok(v) => v,
            Err(e) => panic!("Invalid UTF-8 sequence: {}", e),
        };
        println!("Received message from client: {}", s);
    }
}
