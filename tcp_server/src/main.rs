use std::net::{TcpListener, TcpStream};
use std::io::{Read, Write};

fn main() {
    let listener = TcpListener::bind("127.0.0.1:8080").unwrap();
    println!("Server listening on port 8080");

    for stream in listener.incoming() {
        let mut stream = stream.unwrap();
        println!("Client connected");

        let response = "Hello from Rust!\n";
        stream.write(response.as_bytes()).unwrap();
        println!("Message sent to client");

        let mut buffer = [0; 512];
        stream.read(&mut buffer).unwrap();
        println!("Received message from client: {:?}", buffer);
    }
}
