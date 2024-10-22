#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;

void thread_read(int socketC);

int main() {
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(AF_INET, SOCK_STREAM, 0);
    int n;
    char buffer[256];

    if (-1 == SocketFD) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

    if (0 > Res) {
        perror("error: first parameter is not a valid address family");
        close(SocketFD);
        exit(EXIT_FAILURE);
    } else if (0 == Res) {
        perror("char string (second parameter does not contain valid ipaddress");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    int socketClient = connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

    if (-1 == socketClient) {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    std::thread(thread_read, SocketFD).detach();

    cout << "Log in: ";
    char userBuff[10000];
    fgets(userBuff, 10000, stdin);

    char userBuffSize[5];
    sprintf(userBuffSize, "%04d", ((int)strlen(userBuff)));

    std::string test = "L" + std::string(userBuffSize) + std::string(userBuff);

    write(SocketFD, test.c_str(), test.size() + 1);

    for (;;) {
        char receiver[10000];
        char size_m[5];
        char msg[10000];

        cout << "Log Out (O) - Message (N) - List (I) - Enviar archivo (W) - Tres en Raya (R): ";
        char type[1];
        fgets(type, 10000, stdin);

        switch (type[0]) {
            case 'N': {
                cout << "Send to: ";
                fgets(receiver, 10000, stdin);
                receiver[strlen(receiver) - 1] = '\0';

                printf("Message: ");
                fgets(msg, 10000, stdin);
                msg[strlen(msg) - 1] = '\0';
                sprintf(size_m, "%04d", ((int)strlen(msg)));
                sprintf(userBuffSize, "%04d", ((int)strlen(receiver)));

                string temp = "N" + string(size_m) + string(msg) + string(userBuffSize) + string(receiver);
                write(SocketFD, temp.c_str(), temp.size() + 1);
                break;
            }
            case 'R': {
                // Enviar solicitud de juego
                cout << "Send to (opponent): ";
                fgets(receiver, 10000, stdin);
                receiver[strlen(receiver) - 1] = '\0';

                // Construir mensaje de protocolo 'R' con el nombre del receptor
                sprintf(userBuffSize, "%04d", ((int)strlen(receiver)));
                string temp = "R" + string(userBuffSize) + string(receiver);
                write(SocketFD, temp.c_str(), temp.size() + 1);
                break;
            }

            case 'O': {
                cout << "Logout" << endl;
                write(SocketFD, "O", strlen("O"));
                shutdown(SocketFD, SHUT_RDWR);
                close(SocketFD);
                return 0;
            }

            case 'I': {
                cout << "Getting user list..." << endl;
                write(SocketFD, "I", strlen("I"));
                break;
            }

            case 'W': {
                // solicitar el nombre del archivo
                cout << "Nombre del archivo: ";
                char filePath[10000];
                fgets(filePath, 10000, stdin);
                filePath[strlen(filePath) - 1] = '\0';

                FILE *file = fopen(filePath, "rb");
                if (file == NULL) {
                    cerr << "Error al abrir el archivo" << endl;
                    break;
                }

                fseek(file, 0, SEEK_END);
                long fileSize = ftell(file);
                rewind(file);
                
                string fileName = string(filePath).substr(string(filePath).find_last_of("/") + 1);
                char fileNameSize[5];
                sprintf(fileNameSize, "%04d", (int)fileName.length());

                write(SocketFD, "W", 1);
                
                write(SocketFD, fileNameSize, 4);

                write(SocketFD, fileName.c_str(), fileName.length());

                char fileSizeStr[9];
                sprintf(fileSizeStr, "%08ld", fileSize);
                write(SocketFD, fileSizeStr, 8);

                char buffer[1024];
                long totalBytesSent = 0;
                while (totalBytesSent < fileSize) {
                    size_t bytesRead = fread(buffer, sizeof(char), sizeof(buffer), file);
                    write(SocketFD, buffer, bytesRead);
                    totalBytesSent += bytesRead;
                }

                cout << "Archivo enviado correctamente." << endl;
                fclose(file);
                break;
            }

            default:
                break;
        }
    }

    shutdown(SocketFD, SHUT_RDWR);
    close(SocketFD);
    return 0;
}
void displayBoard(char board[3][3]) {
    cout << "Tablero:" << endl;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            cout << board[i][j] << " ";
        }
        cout << endl;
    }
}
void makeMove(int socketFD, char playerSymbol) {
    int row, col;
    cout << "Coloca tu fila y columna (1-3): ";
    cin >> row >> col;
    row--; col--; 

    char move[5];
    sprintf(move, "%d%d%c", row, col, playerSymbol);
    write(socketFD, move, 3);
}

void thread_read(int socketC) {
    char buffer[10000];
    int n;
    while (true) {
        n = read(socketC, buffer, 1);
        buffer[1] = '\0';

        if (buffer[0] == 'N') {
            n = read(socketC, buffer, 4);
            buffer[4] = '\0';
            int size_msg = atoi(buffer);

            n = read(socketC, buffer, size_msg);
            buffer[size_msg] = '\0';
            char *msg = (char*)malloc(sizeof(char) * size_msg);
            sprintf(msg, "%s", buffer);

            n = read(socketC, buffer, 4);
            buffer[4] = '\0';
            int size_user = atoi(buffer);

            n = read(socketC, buffer, size_user);
            buffer[size_user] = '\0';

            char *cli = (char*)malloc(sizeof(char) * size_user);
            sprintf(cli, "%s", buffer);

            cout << "Message from " << cli << ": " << msg << endl;
        }
        if (buffer[0] == 'I') {
            n = read(socketC, buffer, 4);
            buffer[4] = '\0';
            int size_user = atoi(buffer);

            n = read(socketC, buffer, size_user);
            buffer[size_user] = '\0';

            cout << "Online User: " << buffer << endl;
        }
        if (buffer[0] == 'R') {
            char board[3][3] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}};
            char playerSymbol = 'X';
            bool isMyTurn = true; // Determina si es el turno del jugador local

            displayBoard(board);

            while (true) {
                if (isMyTurn) {
                    cout << "Tu turno!" << endl;
                    makeMove(socketC, playerSymbol);
                }

                // Recibir el movimiento del oponente
                char move[5];
                read(socketC, move, 3);
                int row = move[0] - '0';
                int col = move[1] - '0';
                char opponentSymbol = move[2];

                board[row][col] = opponentSymbol;
                displayBoard(board);

                isMyTurn = !isMyTurn;
            }
        }
    }
}

