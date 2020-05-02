# Internet Banking

## What is Internet Banking
According to [Wikipedia](https://en.wikipedia.org/wiki/Online_banking), Internet banking software provides personal and corporate banking services offering features such as viewing account balances, obtaining statements, checking recent transactions, transferring money between accounts, and making payments.

This project simulates an internet banking application. It was developed in the `Linux` environment and it is a `client-server` application that uses sockets.

## Build
The repo offers only the `.c` source files. So, for testing the project you will need first to build it. You can do that with the `make` command which will build (compile) the files according to the `Makefile` file.

## Behavior Server
You will have to start the server on a separate terminal.
`./server <port_server> <users_data_file>`   
`<users_data_file>` is a file that specifies the clients from the bank. It has the following format:

    <number_of_clients>
    <client1_surname> <client1_name> <card_no> <pin> <password> <balance>
    <client2_surname> <client2_name> <card_no> ...
    ...
      
### Commands
The implemented commands are the following:
* `a` - **tick**, time passes

## How to run it


## Have fun and keep on coding! :)
