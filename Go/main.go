package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"log"
	"net"
	"strconv"
	"strings"
)

var (
	OK = []byte("HTTP/1.1 200 OK\r\n")
	BADREQUEST = []byte("HTTP/1.1 400 Bad Request\r\n")

	GET = map[string]handler{
		"/":homeHandler,
		"/hello":helloHandler,
	}

	POST = map[string]handler{
		"/users":createUserHandler,
	}
)

type HTTPRequest struct {
	Method 			string
	Path 			string
	Version 		string
	Headers 		map[string]string
	Body 			[]byte
	ReturnsContent 	bool
}

type HTTPResponse struct {
	StatusLine 	[]byte
	Headers 	map[string]string
	Body		[]byte
}

type handler func(*HTTPRequest) *HTTPResponse

func homeHandler(_ *HTTPRequest) *HTTPResponse {
	return &HTTPResponse{
		StatusLine: OK,
		Headers: make(map[string]string),
		Body: []byte("HTTP Server in GoLang exercise, nothing exciting here"),
	}
}

func helloHandler(_ *HTTPRequest) *HTTPResponse {
	return &HTTPResponse{
		StatusLine: OK,
		Headers: make(map[string]string),
		Body: []byte("Hello to you too!"),
	}
}

func createUserHandler(req *HTTPRequest) *HTTPResponse {
	response := &HTTPResponse{
		Headers: make(map[string]string),
	}

	response.StatusLine = OK

	userData := strings.Split(string(bytes.Trim(req.Body, "{\"}")), ":")
	name := strings.Trim(userData[1], " \"")

	response.Body = fmt.Appendf(response.Body, "User %v successfully created!", name)
	return response
}

func ParseHTTPRequest(c net.Conn) (*HTTPRequest, error) {
	reader := bufio.NewReader(c)
	
	req := &HTTPRequest{
		Headers: make(map[string]string),
	}

	requestLine, err := reader.ReadBytes('\n')

	if err != nil {
		return nil, err
	}

	requestLine = bytes.TrimRight(requestLine, "\r\n")
	splitString := strings.Split(string(requestLine), " ")

	req.Method = splitString[0]
	req.Path = splitString[1]
	req.Version = splitString[2]

	for {
		headerLine, err := reader.ReadBytes('\n')

		if err != nil {
			return nil, err
		}

		if bytes.Equal(headerLine, []byte("\r\n")) {
			break
		}

		headerLine = bytes.TrimRight(headerLine, "\r\n")
		splitString = strings.Split(string(headerLine), ": ")
		
		req.Headers[splitString[0]] = splitString[1]
	}

	switch req.Method {
	case "GET":
		req.ReturnsContent = true

	case "POST", "PUT":
		numBytes := req.Headers["Content-Length"]
		len, err := strconv.Atoi(numBytes)

		if err != nil {
			return nil, err
		}

		buf := make([]byte, len)
		_, err = io.ReadFull(reader, buf)

		if err != nil {
			return nil, err
		}

		req.Body = buf
		req.ReturnsContent = true
	}

	return req, nil
}

func WriteHTTPResponse(c net.Conn, responseData *HTTPResponse, returnsContent bool) {
	var response []byte

	response = append(response, responseData.StatusLine...)

	for k, v := range responseData.Headers {
		response = append(response, (k + v)...)
	}
	response = append(response, []byte("\r\n")...)

	if returnsContent {
		response = append(response, responseData.Body...)
	}

	_, err := c.Write(response)

	if err != nil {
		log.Fatal(err)
	}

}

func handleConnection(c net.Conn) {
	fmt.Println("Connection has been accepted successfully")

	req, err := ParseHTTPRequest(c)

	if err != nil {
		//WriteHTTPResponse(c, BADREQUEST)
		log.Fatal(err)
	}

	var hf handler

	if req.Method == "GET" {
		hf = GET[req.Path]
	} else if req.Method == "POST" {
		hf = POST[req.Path]
	} else {
		
	}

	response  := hf(req)
	responseBodyLen := len(response.Body)

	if responseBodyLen > 0 {
		response.Headers["Content-Length: "] = strconv.Itoa(responseBodyLen) + "\r\n"
	}

	fmt.Printf("%v", req)

	WriteHTTPResponse(c, response, req.ReturnsContent)
}

func main() {
	fmt.Println("Initialising the HTTP Server")

	ln, err := net.Listen("tcp", ":8080")

	if err != nil {
		log.Fatal(err)
	}

	for {
		conn, err := ln.Accept()

		if err != nil {
			log.Fatal(err)
		}

		go handleConnection(conn)
	}
}
