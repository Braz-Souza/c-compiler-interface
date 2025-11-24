# c-compiler-interface

## Compilação

### Server
```bash
gcc -o server server.c
```

### Client (com GTK3)
```bash
gcc -o client client.c `pkg-config --cflags --libs gtk+-3.0`
```

## Execução

### 1. Iniciar o servidor
```bash
./server 8800
```

### 2. Iniciar o client (interface GTK)
```bash
./client localhost 8800
```