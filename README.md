# HTTP Servers

A collection of HTTP servers built from scratch in different languages as a learning exercise. Each implementation handles the protocol manually — no web frameworks — to explore how HTTP works at the byte level and how each language approaches systems programming.

## Implementations

- **Go** — `Go/`
- **Rust** — `Rust/http-server/`
- **C++** — `C++/`

Each server implements HTTP/1.1 with request parsing, basic routing (GET and POST), and response generation.

## Scope

These are learning projects, not production servers. They handle the happy path and a handful of error cases. JSON parsing is ad-hoc rather than via a real parser, routing is exact-match only, and concurrency is minimal. The goal is understanding the protocol and the language, not building something deployable.

## Running

Each implementation lives in its own directory with its own build instructions. All servers listen on `127.0.0.1:8080` by default.

Test with curl:

```bash
curl -v http://localhost:8080/
curl -v http://localhost:8080/hello
curl -v -X POST http://localhost:8080/users -d '{"name": "Alice"}'
```
