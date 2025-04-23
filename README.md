# 📘 CAPIS — Simple and Fast API Test Tool

**`capis` is a lightweight, fast, and developer-friendly API testing tool based on `libyaml` and `libcurl`.**

Just describe your API test cases using YAML — it's simple and intuitive. Let’s dive in step by step.

------

## 🚀 Getting Started

### ✅ Example: Simple GET Request

```yml
method: GET
url: https://github.com
secure: false
```

Run it with:

```bash
capis ./test.yml
```

This sends a `GET` request to GitHub.

> Setting `secure: false` disables SSL verification, which is helpful for debugging (but unsafe for production).

Sample output:

```
[INFO]  CAPIS RUNNING
[INFO]  Processing METADATA: ./test.yml
[INFO]  Preparing request: https://github.com
[WARN]  SSL verification disabled - security risk
[INFO]  Request successful - Status Code: 200
[INFO]  ========== RESPONSE HEADERS ==========
...
[INFO]  ========== RESPONSE BODY =============
<html> ... </html>
...
[INFO]  Request completed for ./test.yml
```

To show detailed debug logs, use `-v` or `--verbose`:

```bash
capis ./test.yml -v
```

It will print out the request details, connection status, SSL certificate info, etc.

------

## 🔁 Example: POST Request with Parameters

```yml
name: login to mall
method: POST
host: localhost:8080
path: /user/login
params:
  email: 987654321@gmail.com
  password: 123456
```

You don’t need to specify `url` if `host` and `path` are provided.

> The `name` field is optional and ignored by the parser — use it for documentation or grouping.

------

## 🍪 Set Headers and Cookies

```yml
description: query goods
method: GET
url: http://localhost:8080/goods/info/book
cookie:
  - key: token
    value: asdfghjkl
    httpOnly: true
    secure: true
    domain: localhost
    path: /
  - key: SSID
    value: AOc5fvODcakyI6P_c
headers:
  user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 ...
secure: false
timeout: 10000  # in milliseconds
```

This sends a GET request with cookies and custom headers, bypassing SSL verification and using a 10-second timeout.
