use std::{io::{BufRead, BufReader, Read}, net::{Shutdown, TcpListener, TcpStream}, sync::LazyLock};
use anyhow::{Context, Ok, Result};
use std::collections::HashMap;

#[derive(Default, Debug)]
struct HTTPRequest {
    method: String,
    path:   String,
    version:    String,
    headers:    HashMap<String, String>,
    body:      Vec<u8> 
}

struct HTTPResponse {
    status_line: Vec<u8>,
    headers:    HashMap<String, String>,
    body:       Vec<u8>
}

type Handler = fn(&HTTPRequest) -> Result<HTTPResponse>;

static GET: LazyLock<HashMap<&str, Handler>> = LazyLock::new(|| { 
    HashMap::from([
        ("/", home_handler as Handler),
        ("/hello", hello_handler as Handler),
    ])
});


static POST: LazyLock<HashMap<&str, Handler>> = LazyLock::new(|| { 
    HashMap::from([
        ("/users", create_user as Handler),
    ])
});

fn home_handler(req: &HTTPRequest) -> Result<HTTPResponse> {



    Ok(HTTPResponse {

    })
}

fn hello_handler(req: &HTTPRequest) -> Result<HTTPResponse> {

    Ok(HTTPResponse {

    })
}

fn create_user(req: &HTTPRequest) -> Result<HTTPResponse> {

    Ok(HTTPResponse {

    })
}

fn parse_http_request(stream: &mut TcpStream) -> Result<HTTPRequest> {
    let mut reader = BufReader::new(stream);
   
    let mut line = String::new();

    reader.read_line(&mut line)?;

    let mut parsed_header_iter = line.split_whitespace();

    let method = parsed_header_iter.next().context("Request line missing method")?.to_string();
    let path = parsed_header_iter.next().context("Request line missing path")?.to_string();
    let version = parsed_header_iter.next().context("Request line missing version")?.to_string();

    let mut headers = HashMap::new();
    let mut body = vec![];


    loop {
        line.clear();
        reader.read_line(&mut line)?;

        if line.eq("\r\n") {
            break;
        }

        let split_header = line.trim().split_once(":").context("Header missing context")?;
        headers.insert(split_header.0.trim().to_string(), split_header.1.trim().to_string());
    }

    if let Some(length) = headers.get("Content-Length") {
        let mut buf = Vec::with_capacity(length.parse().context("Content length is malformed")?);
        reader.read_exact(&mut buf)?;

        body = std::mem::take(&mut buf);
    }
    
    Ok(HTTPRequest {
        method,
        path,
        version,
        headers,
        body,
    })


}

fn write_http_response() {

}

fn handle_connection(stream: &mut TcpStream) -> Result<()> {
    println!("Client successfully connected!");

    let req =    parse_http_request(stream)?;
    dbg!(&req); 



    stream.shutdown(Shutdown::Both)?;
    Ok(())
}



fn main() -> Result<()>{
    println!("Starting HTTP Server...");
    println!("Waiting for client to connect.");

   let listener = TcpListener::bind("127.0.0.1:8080").context("Failed to bind to port 8080")?;

    for stream in listener.incoming() {
        handle_connection(&mut stream?)?;
    }

    
    Ok(())
}
