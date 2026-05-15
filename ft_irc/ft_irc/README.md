*This project has been created as part of the 42 curriculum by luxu, aicoulib.*

# ft_irc

## Description

`ft_irc` is a network programming project whose goal is to build a simple Internet Relay Chat (IRC) server in C++.

The server must be able to handle multiple clients at the same time, allow them to authenticate, choose nicknames, join channels, exchange private and channel messages, and manage channel permissions through a subset of the IRC protocol.

This project focuses on several core topics:

- socket programming
- TCP/IP communication
- multiplexing with `poll()`
- non-blocking I/O
- state management for connected clients and channels
- command parsing and protocol design
- robust error handling and connection cleanup

The final result is a lightweight IRC server executable that can be tested with tools such as `nc` or with a minimal custom IRC client.


## Instructions
This section explains how to compile, run, and manually test the server with/without the given client tester.

### Compilation and Execution
Build the server with:
```bash
make
```

Clean the project with:

```bash
make clean
make fclean
make re
```

Run the server with:

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 pass
```

### Using `nc` for Manual Testing

`nc` (netcat) is a lightweight command-line networking tool that can open a TCP connection to the IRC server and send raw text commands directly from the terminal. It is especially useful during development because it allows manual testing of the server without requiring a full IRC client.

In this project, `nc` can be used to:

- verify that the server accepts TCP connections
- manually test the registration sequence
- test IRC commands one by one
- check server responses and error handling
- simulate partial command delivery during evaluation

#### Basic registration example

After connecting, a client can register with:

```text
PASS pass
NICK aaa
USER user 0 * :realname
```

#### Example commands

Once registered, common commands can be tested manually:

```text
JOIN #test
PRIVMSG #test :hello everyone
TOPIC #test :new topic
MODE #test +i
```

#### Partial input test

The project subject explicitly mentions testing fragmented input. With `nc -C`, a command can be sent in multiple parts using `Ctrl+D`, for example:

- type `com`, then press `Ctrl+D`
- type `man`, then press `Ctrl+D`
- type `d`, then press Enter

This simulates a command arriving in multiple packets and helps verify that the server correctly rebuilds a full command from partial received data before parsing it.

#### Client suspend test (Ctrl + Z)

Create a client A, join a channel, type `ctrl + z` to suspend the nc process;
Then create client B with the following command:

```text
(
echo "PASS pass"
echo "NICK bbb"
echo "USER bbb bbb localhost :bbb"
echo "JOIN #1"
while true; do echo "PRIVMSG #1 :hello hello hello hello hello"; done
) | nc 127.0.0.1 6667
```
Then use `fg` for client A to continue the session.
The client A should be able to receive all the messages flushed by the send buffer.


### Using the Provided Test Client

In addition to `nc`, this project also includes a minimal custom test client designed to validate behaviors that are harder to observe manually, especially non-blocking output handling, long message delivery, and slow-client scenarios.

This test client can be used to:

- automate registration and channel joining
- send long or reduced-length long messages
- simulate slow receivers
- simulate clients that barely read or do not read incoming data
- summarize long-message reception instead of printing the full payload
- manually interact with the server in interactive mode

#### Compilation

If the client tester is stored in its own directory and has its own `Makefile`, compile it with:

```bash
make
```

#### General usage

The test client is executed as follows:

```bash
./test_client <host> <port> <password> <nick> <user> <mode> [channel]
```

#### Available modes

```text
normal        - receive and print messages normally
sendlong      - automatically send extra-long channel messages
sendshortlong - send one shorter long message only
slow          - read server messages slowly
noread        - barely read any server messages
interactive   - receive normally and allow continued input
summary       - receive and print compact message summaries
slowsummary   - read slowly and print compact summaries
```

#### Example commands

Connect as a normal client:

```bash
./test_client 127.0.0.1 6667 pass aaa user normal #1
```

Send repeated extra-long messages:

```bash
./test_client 127.0.0.1 6667 pass ccc user sendlong #1
```

Send one shorter long message:

```bash
./test_client 127.0.0.1 6667 pass hhh user sendshortlong #1
```

Observe incoming long messages through compact summaries:

```bash
./test_client 127.0.0.1 6667 pass fff user summary #1
```

Simulate a slow receiver while still printing summaries:

```bash
./test_client 127.0.0.1 6667 pass ggg user slowsummary #1
```

Use an interactive client that continues to accept terminal input after registration:

```bash
./test_client 127.0.0.1 6667 pass eee user interactive #1
```

#### Why this client is useful

Unlike `nc`, this custom tester makes it easier to reproduce stress cases relevant to this project, such as:

- partial or delayed reception
- very long outgoing messages
- slow clients creating send-buffer pressure
- checking whether long messages are still received completely when one client reads much more slowly than another

It is therefore especially useful for validating the server's non-blocking behavior, output buffering, and `POLLOUT`-based continuation logic.


### Using `irssi` for Interactive IRC Testing

`irssi` is a full-featured terminal IRC client that can connect directly to the server and interact with it using real IRC workflows. Unlike `nc`, it automatically handles registration-related commands and provides a much more realistic client-side testing environment.

Using `irssi` is especially useful for:

- validating compatibility with a real IRC client
- testing channel interaction behavior
- testing private messaging and broadcasts
- observing operator permissions and mode updates
- checking disconnect and reconnect behavior
- testing multiple simultaneous users through multiple terminals

#### Installation

On macOS:

```bash
brew install irssi
```

On Debian/Ubuntu:

```bash
sudo apt install irssi
```

#### Starting irssi

Launch the client with:

```bash
irssi
```

Or specify a nickname directly:

```bash
irssi -n aaa
```

#### Connecting to the server

Inside `irssi`, connect with:

```irc
/connect 127.0.0.1 6667 pass
```

The client will automatically send commands such as:

```text
CAP LS 302
NICK aaa
USER aaa aaa 127.0.0.1 :Real Name
```

The server should correctly handle or ignore these commands and complete registration with numeric reply `001`.

#### Common irssi commands

Join a channel:

```irc
/join #1
```

Send a channel message:

```text
hello everyone
```

Send a private message:

```irc
/msg bbb hello
```

Change nickname:

```irc
/nick newnick
```

Kick a user:

```irc
/kick #1 bbb testing
```

Invite a user:

```irc
/invite bbb #1
```

Quit the server:

```irc
/quit
```

#### Window navigation

`irssi` uses multiple windows internally.

Useful commands:

```irc
/window list
/window goto 1
/window goto 2
```

Useful shortcuts:

```text
Alt + 1    -> status window
Alt + 2    -> first channel window
Alt + 3    -> second channel/private window
```

IRC errors and numeric replies are often displayed in the status window rather than the current channel window.

#### Typical testing workflow

Terminal 1:

```bash
./ircserv 6667 pass
```

Terminal 2:

```bash
irssi -n aaa
```

Inside irssi:

```irc
/connect 127.0.0.1 6667 pass
/join #1
```

Terminal 3:

```bash
irssi -n bbb
```

Inside irssi:

```irc
/connect 127.0.0.1 6667 pass
/join #1
```

The two clients can then be used to test:

- `PRIVMSG`
- `KICK`
- `INVITE`
- `TOPIC`
- `MODE`
- disconnect handling
- channel broadcasts
- operator permissions


## Main Goals

The main goals of this project are:

- to understand how a TCP server works
- to manage multiple simultaneous client connections without using threads
- to implement a subset of IRC commands and channel operator features
- to maintain a stable event loop using `poll()`
- to correctly handle client registration, disconnection, and channel state cleanup

## Features

This project implements the mandatory IRC server features required by the subject, including:

### Registration
- `PASS`
- `NICK`
- `USER`

### Communication
- `PRIVMSG` to another user
- `PRIVMSG` to a channel

### Channel management
- `JOIN`
- channel creation
- topic display on join
- channel membership tracking
- operator tracking
- invite tracking

### Operator commands
- `KICK`
- `INVITE`
- `TOPIC`
- `MODE`

### Supported channel modes
- `+i / -i` — invite-only channel
- `+t / -t` — only operators may change the topic
- `+k / -k` — channel password
- `+o / -o` — give/take operator privileges
- `+l / -l` — member limit

## Technical Overview

### Server architecture
The server is built around a non-blocking socket and a `poll()`-based event loop.

Its main responsibilities are:

- creating and configuring the listening socket
- accepting new client connections
- reading incoming data into per-client buffers
- splitting raw input into full IRC lines
- parsing commands and dispatching them to handlers
- maintaining outgoing send buffers
- continuing partial sends through `POLLOUT`
- cleaning channel state when a client disconnects

### Client state
Each client stores its own connection and registration state, including:

- file descriptor
- IP address
- nickname
- username
- receive buffer
- send buffer
- password status
- registration status
- disconnect flag

### Channel state
Each channel stores its own runtime state, including:

- channel name
- topic
- member list
- operator list
- invited users
- invite-only state
- topic restriction state
- channel key
- member limit

## Project Structure

Example structure:

```text
.
├── README.md
├── Makefile
├── main.cpp
├── Server.hpp
├── Client.hpp
├── Channel.hpp
├── Client.cpp
├── Channel.cpp
├── Commands.cpp
├── Mode.cpp
├── ServerAuth.cpp
├── ServerChannel.cpp
├── ServerCore.cpp
└── ServerIO.cpp
·
```

## Resources

The following resources were used throughout the project to understand IRC behavior, socket programming, non-blocking I/O, and expected protocol semantics.

### Classic references

- RFC 1459 — *Internet Relay Chat Protocol*
- RFC 2810 — *Internet Relay Chat: Architecture*
- RFC 2811 — *Internet Relay Chat: Channel Management*
- RFC 2812 — *Internet Relay Chat: Client Protocol*
- Beej's Guide to Network Programming
- POSIX / Unix manual pages:
  - `man 2 socket`
  - `man 2 bind`
  - `man 2 listen`
  - `man 2 accept`
  - `man 2 send`
  - `man 2 recv`
  - `man 2 poll`
  - `man 2 fcntl`
  - `man 7 tcp`

### Learning the expected IRC behavior

A significant part of the work involved studying how a standard IRC server is expected to behave in practice. This included comparing command semantics, permission rules, and channel-event behavior with classic IRC references and online explanations.

Particular attention was given to:

- registration flow with `PASS`, `NICK`, and `USER`
- channel joining rules, including invite-only, key-protected, and limited channels
- operator permissions and channel mode changes
- topic management rules and visibility
- expected behavior of channel broadcasts for events such as `JOIN`, `KICK`, `MODE`, `TOPIC`, and nickname changes
- fragmented input, partial delivery, and reconstruction of commands from multiple received packets

This research was used to align the implementation with the standard IRC model as closely as possible within the scope of the 42 project.

### AI usage

AI was used as a learning and review tool during the project, not as a substitute for implementation ownership.

It was mainly used for:

- clarifying IRC protocol rules and standard command behavior
- comparing different implementation choices for channel state and command handling
- reviewing architecture decisions, especially around client state, channel state, and command dispatching
- discussing non-blocking design, `poll()`-based event loops, receive/send buffering, and partial-send handling
- identifying edge cases in commands such as `JOIN`, `MODE`, `TOPIC`, `INVITE`, `KICK`, and `PRIVMSG`
- improving test strategy, including long-message testing, slow-client testing, and connection cleanup scenarios
- refining documentation structure and README wording

AI assistance was especially useful for:

- learning network-programming concepts that were new during the project
- validating design ideas before implementation
- spotting state-management inconsistencies and cleanup issues
- improving code organization and architectural clarity during refactoring

All final code integration, debugging, behavioral validation, and protocol testing were performed manually.

### Notes on external observation and testing

Besides documentation and manual pages, online material was also used to observe how IRC systems typically behave from a user perspective. This helped compare the implemented server against common IRC expectations, especially for:

- channel event visibility
- operator notifications and broadcasts
- repeated command behavior
- reconnect and cleanup semantics
- practical testing with tools such as `nc` and a custom test client
