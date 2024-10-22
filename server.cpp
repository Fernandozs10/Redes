#include <bits/stdc++.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <map>

using namespace std;

map<string, int> users;
map<int, string> sockets;

bool checkWinner(char board[3][3]) {
    for (int i = 0; i < 3; ++i) {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2])
            return true;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i])
            return true;
    }
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2])
        return true;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0])
        return true;
    return false;
}
void manageTicTacToe(int player1, int player2) {
    cout << "si llego"<< endl;
    char board[3][3] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}};
    int currentPlayer = player1;
    int otherPlayer = player2;
    char symbols[2] = {'X', 'O'};
    
    while (true) {
        char move[5];
        
        if (read(currentPlayer, move, 3) <= 0) {
            cout << "Error reading from player " << currentPlayer << endl;
            break;
        }
        int row = move[0] - '0';
        int col = move[1] - '0';
        char symbol = move[2];
        
        board[row][col] = symbol;
        
        write(otherPlayer, move, 3);
        
        if (checkWinner(board)) {
            cout << "Jugador " << currentPlayer << " gano!" << endl;
            break;
        }
        
        swap(currentPlayer, otherPlayer);
    }
}


void thread_read(int socketC)
{
    int n;
    char buffer[10000];

    do {
        bzero(buffer, 255);
        n = read(socketC, buffer, 1);
        buffer[1] = '\0';

        char protocol = buffer[0];

        switch (protocol) {
            case 'L': { // Login
                n = read(socketC, buffer, 4);
                buffer[4] = '\0';
                int userBuffSize = atoi(buffer);
                n = read(socketC, buffer, userBuffSize + 1);
                buffer[userBuffSize - 1] = '\0';

                cout << "Login: " << buffer << endl;
                users.insert({buffer, socketC});
                sockets.insert({socketC, buffer});
                break;
            }

            case 'N': { // Enviar mensaje a otro usuario
                n = read(socketC, buffer, 4);
                buffer[4] = '\0';
                int size_m = atoi(buffer);
                n = read(socketC, buffer, size_m);
                buffer[size_m] = '\0';
                char *msg = (char*)malloc(sizeof(char) * size_m);
                sprintf(msg, "%s", buffer);

                n = read(socketC, buffer, 4);
                buffer[4] = '\0';
                int userBuffSize = atoi(buffer);
                n = read(socketC, buffer, userBuffSize);
                buffer[userBuffSize] = '\0';

                char *cli = (char*)malloc(sizeof(char) * userBuffSize);
                sprintf(cli, "%s", buffer);

                int rec = users[buffer];
                char user2[10000];
                sprintf(user2, "%s", sockets[socketC].c_str());

                char csize_m[5];
                sprintf(csize_m, "%04d", size_m);

                char cszuser2[5];
                int szuser2 = strlen(user2);
                sprintf(cszuser2, "%04d", szuser2);

                cout << "Message from " << sockets[socketC] << " to " << cli << ": " << msg << endl;

                write(rec, "N", strlen("N"));
                write(rec, csize_m, strlen(csize_m));
                write(rec, msg, strlen(msg));
                write(rec, cszuser2, strlen(cszuser2));
                write(rec, user2, strlen(user2));

                cout << "Message sent." << endl;
                break;
            }

            case 'W': {

                n = read(socketC, buffer, 4);
                buffer[4] = '\0';
                int fileNameSize = atoi(buffer);


                n = read(socketC, buffer, fileNameSize);
                buffer[fileNameSize] = '\0';
                string fileName = buffer;


                FILE *file = fopen(fileName.c_str(), "wb");
                if (file == NULL) {
                    cerr << "Error al abrir archivo en el servidor" << endl;
                    break;
                }


                n = read(socketC, buffer, 8);
                buffer[8] = '\0';
                long fileSize = atol(buffer);

                cout << "Recibiendo archivo: " << fileName << " de tamaño " << fileSize << " bytes." << endl;


                long totalBytesReceived = 0;
                while (totalBytesReceived < fileSize) {
                    n = read(socketC, buffer, sizeof(buffer));
                    fwrite(buffer, sizeof(char), n, file);
                    totalBytesReceived += n;
                }

                cout << "Archivo recibido correctamente." << endl;
                fclose(file);
                break;
            }
            case 'R': { // Iniciar tres en raya
                n = read(socketC, buffer, 4);
                buffer[4] = '\0';
                int userBuffSize = atoi(buffer);
                n = read(socketC, buffer, userBuffSize);
                buffer[userBuffSize] = '\0';

                int opponentSocket = users[buffer];

                if (opponentSocket) {
                    cout << "Empezando 3 en raya entre " << socketC << " y " << opponentSocket << endl;
                    manageTicTacToe(socketC, opponentSocket);
                } else {
                    cerr << "Opponent not found." << endl;
                }
                break;
            }

            case 'O': { // Logout
                cout << "LogOut " << sockets[socketC] << endl;
                users.erase(sockets[socketC]);
                sockets.erase(socketC);
                break;
            }

            case 'I': { // Lista de usuarios conectados
                cout << "Users List" << endl;
                map<int, string>::iterator it;
                for (it = sockets.begin(); it != sockets.end(); it++) {
                    const void *nombre = ((*it).second).c_str();
                    char csize_nombre[5];
                    int szuser = ((*it).second).length();
                    sprintf(csize_nombre, "%04d", szuser);

                    write(socketC, "I", 1);
                    write(socketC, csize_nombre, 4);
                    write(socketC, nombre, ((*it).second).length());
                }
                break;
            }

            default:
                break;
        }
    } while (strcmp(buffer, "Bye") != 0);
}

int main() {
    struct sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buffer[256];
    int n;

    if (-1 == SocketFD) {
        perror("Can not create socket");
        exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    if (-1 == bind(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in))) {
        perror("Error bind failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(SocketFD, 10)) {
        perror("Error listen failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    for (;;) {
        int ConnectFD = accept(SocketFD, NULL, NULL);

        if (0 > ConnectFD) {
            perror("Error accept failed");
            close(SocketFD);
            exit(EXIT_FAILURE);
        }

        std::thread(thread_read, ConnectFD).detach();
    }

    close(SocketFD);
    return 0;
}
