use std::{io::Read, net::{Shutdown, TcpListener, TcpStream}};
use anyhow::{Context, Ok, Result};
use std::collections::HashMap;

#[derive(Default)]
struct HTTPRequest {
    method: String,
    path:   String,
    version:    String,
    headers:    HashMap<String, String>,
    body:       u8
}

fn parse_http_request(stream: &mut TcpStream) -> Result<HTTPRequest> {
    let mut req = HTTPRequest::default();


    let mut buf = vec![];
    let num_bytes_read = stream.read_to_end(&mut buf)?;

    //TODO: Turn the buf into a string OR manually read till whitespace, then get the index of
    //whitespace and get a slice of the vec, removing the slice and appending to req.
    Ok(req)
}

fn handle_client(stream: &mut TcpStream) -> Result<()> {
    println!("Client successfully connected!");

    parse_http_request(stream)?;
    

    //stream.shutdown(Shutdown::Both)?;
    Ok(())
}
fn main() -> Result<()>{
    println!("Starting HTTP Server...");
    println!("Waiting for client to connect.");

   let listener = TcpListener::bind("127.0.0.1:8080").context("Failed to bind to port 8080")?;

    for stream in listener.incoming() {
        handle_client(&mut stream?)?;
    }

    
    Ok(())
}
