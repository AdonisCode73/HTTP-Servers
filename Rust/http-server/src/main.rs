use std::{fmt::format, io::{BufRead, BufReader, Read, Write}, net::{Shutdown, TcpListener, TcpStream}, sync::LazyLock};
use anyhow::{Context, Ok, Result};
use std::collections::HashMap;

enum StatusCode {
    Ok,
    BadRequest,
    NotFound,
    MethodNotAllowed,
    InternalServerError,
}

impl StatusCode {
    fn status_line(&self) -> &'static [u8] {
        match self {
            StatusCode::Ok => b"HTTP/1.1 200 OK\r\n",
            StatusCode::BadRequest => b"HTTP/1.1 400 Bad Request\r\n",
            StatusCode::NotFound => b"HTTP/1.1 404 Not Found\r\n",
            StatusCode::MethodNotAllowed => b"HTTP/1.1 405 Method Not Allowed\r\n",
            StatusCode::InternalServerError => b"HTTP/1.1 500 Internal Server Error\r\n",
        }
    }

    fn code(&self) -> u16 {
        match self {
            StatusCode::Ok => 200,
            StatusCode::BadRequest => 400,
            StatusCode::NotFound => 404,
            StatusCode::MethodNotAllowed => 405,
            StatusCode::InternalServerError => 500,
        }
    }
}

#[derive(Default, Debug)]
struct HTTPRequest {
    method: String,
    path:   String,
    version:    String,
    headers:    HashMap<String, String>,
    body:      Vec<u8>
}

struct HTTPResponse {
    status_line: StatusCode,
    headers:    HashMap<String, String>,
    body:       Vec<u8>
}

type Handler = fn(HTTPRequest) -> Result<HTTPResponse>;

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

fn home_handler(_: HTTPRequest) -> Result<HTTPResponse> {
    Ok(HTTPResponse {
        status_line: StatusCode::Ok,
        headers: HashMap::new(),
        body: Vec::from(b"HTTP Server in Rust exercise, nothing exciting here"),
    })
}

fn hello_handler(_: HTTPRequest) -> Result<HTTPResponse> {
    Ok(HTTPResponse {
        status_line: StatusCode::Ok,
        headers: HashMap::new(),
        body: Vec::from(b"Hello to you too!"),
    })
}

fn invalid_path_handler() -> Result<HTTPResponse> {
    Ok(HTTPResponse {
        status_line: StatusCode::NotFound,
        headers: HashMap::new(),
        body: Vec::from(b"Invalid path provided"),
    })
}

fn create_user(req: HTTPRequest) -> Result<HTTPResponse> {
    let body_string = String::from_utf8(req.body)?;

    let split_string = body_string.split_once(":").context("Invalid POST create user body")?;
    let parsed_name = split_string.1.trim_matches(|c: char| c.is_whitespace() || c == '"' || c == '}');
    
    dbg!(parsed_name);
    Ok(HTTPResponse {
        status_line: StatusCode::Ok,
        headers: HashMap::new(),
        body: Vec::from(format!("User {} successfully created!", parsed_name)),
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
        //let mut buf = Vec::with_capacity(length.parse().context("Content length is malformed")?);
        let mut buf = vec![0u8; length.parse().context("Content length is malformed")?];
        reader.read_exact(&mut buf)?;

        body = std::mem::take(&mut buf);
    }
    
    Ok(HTTPRequest {
        method,
        path,
        version,
        headers,
        body
    })


}

fn write_http_response(resp: HTTPResponse, stream: &mut TcpStream, returns_content: bool) -> Result<()> {
    let mut buf: Vec<u8> = Vec::new();

    buf.extend_from_slice(resp.status_line.status_line());

    for (name, value) in &resp.headers {
        let result = name.to_owned() + ": " + value + "\r\n";
        buf.extend_from_slice(result.as_bytes());
    }

    buf.extend_from_slice("\r\n".as_bytes());

    if returns_content {
        buf.extend_from_slice(&resp.body);
    }

    stream.write_all(&buf).context("Failure writing HTTP Response")?;

    Ok(())
}

fn handle_connection(stream: &mut TcpStream) -> Result<()> {
    println!("Client successfully connected!");

    let req =    parse_http_request(stream)?;
    dbg!(&req);


    let mut resp = match req.method.as_str() {
        "GET" => {
            if let Some(handler) = GET.get(req.path.as_str()) {
                handler(req)?
            } else {
                invalid_path_handler()? 
            }
        }
        "POST" => {
            if let Some(handler) = POST.get(req.path.as_str()) {
                handler(req)?
            } else {
                invalid_path_handler()?
            }
        }
        _ => invalid_path_handler()?,
    };

    let mut returns_content = false;

    if resp.body.len() > 0 {
        returns_content = true;
        resp.headers.insert("Content-Length".to_string(), resp.body.len().to_string());
    }

    write_http_response(resp, stream, returns_content)?;

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
