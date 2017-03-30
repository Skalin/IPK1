#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <unistd.h>
#ifdef _WIN32
	#include <Winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif

#include <sstream>
#include <iostream>
#include <vector>
#include <utility>
#include <time.h>
#include <ctime>
#include <fstream>

using namespace std;


/*
 * Function prints a error message on cerr and exits program
 *
 * @param const char *message message to be printed to cerr
 */
void throwException(const char *message) {
	cerr << (message) << endl;
	exit(EXIT_FAILURE);
}

/*
 * Function gets length of the file in bytes
 *
 * @param string file path to the file
 * @return size of the file in bytes
 */
int getBinaryFileLength(string filename) {
	return (sizeof(filename)*8);
}


/*
 * Function loads a file as binary type to string
 *
 * @param path to the file
 * @return file as a string
 */
string loadFile(string filename) {

	string bin = "";

	ifstream fd (filename, ios::binary);
	if (fd.is_open()) {
		ostringstream strf;
		unsigned char c;
		strf << fd.rdbuf();
		while (fd >> c) {
			strf << c;
		}
		bin = (strf.str());
		return bin;
	} else {
		throwException("Error: Couldn't open a file.");
	}

}

/*
 * Function generates current date in a format %a, %d %b %Y %T %Z
 *
 * @return string dateTime containing correct format of date
 */
string getCurrDate() {

	char mbstr[100];
	string dateTime;

	time_t t = time(NULL);
	if (strftime(mbstr, sizeof(mbstr), "%a, %d %b %Y %T %Z", localtime(&t))) {
		dateTime = mbstr;
	} else {
		throwException("Error: Date could not be resolved.\n");
	}

	return dateTime;
}

/*
 * Function returns substring from the given string, by delimiter
 *
 * @param string String which is searched
 * @param string delimiter which is used to split the String
 * @param bool way - true is substr to the right, false is to the left
 */
string returnSubstring(string String, string delimiter, bool way){

	string subString = "";
	if (String.find(delimiter) != string::npos) {
		if (way) {
			subString = String.substr(String.find(delimiter) + delimiter.length());
		} else {
			subString = String.substr(0, String.find(delimiter));
		}
	}

	return subString;
}

/*
 * Function converts string to uppercase
 *
 * @param string String to be converted to uppercase
 * @return string newString converted to uppercase
 */
string toUpper(string String) {
	string newString = String;
	for (unsigned int i = 0; i < String.length(); i++) {
		newString.at(i) = (char)toupper(newString.at(i));
	}

	return newString;
}

int main(int argc, char *argv[]) {

	string cmd;

	if (argc == 3 || argc == 4) {
		if (strcmp(argv[1],"put") != 0) {
			if (argc == 3) {
				if (strcmp(argv[1], "mkd") == 0 || strcmp(argv[1], "lst") == 0 || strcmp(argv[1], "get") == 0 ||
					strcmp(argv[1], "del") == 0 || strcmp(argv[1], "rmd") == 0)
					cmd = toUpper(argv[1]);
				else
					throwException("Error: Bad arguments!\n");
			} else {
				if (strcmp(argv[1], "get") == 0) {
					cmd = toUpper(argv[1]);
				} else {
					throwException("Error: Bad arguments!\n");
				}
			}
		} else {
			if (argc == 4) {
				cmd = toUpper(argv[1]);
			} else {
				throwException("Error: Bad arguments!\n");
			}
		}
	} else {
		throwException("Error: Bad arguments!\n");
	}

	string localFile;
	if (argc == 4 && (cmd == "PUT" || cmd == "GET")) {
		localFile = argv[3];
	}

	string host, hostName, fd, user;
	int port = 0;
	if ((host = returnSubstring(argv[2], "http://", true)) != "") {
		if ((hostName = returnSubstring(host, ":", false)) == "") {
			throwException("Error: Incorrect hostname!\n");
		}
		char *pPort;
		if ((port = strtol(returnSubstring(returnSubstring(host, ":", true),"/", false).c_str(), &pPort, 10)) < 1024 || port > 65535) {
			throwException("Error: Wrong port range!\n");
		}
		if ((fd = returnSubstring(host, "/", true)) == "") {
			throwException("Error: Folder not selected!\n");
		}
	} else {
		throwException("Error: Incorrect host!\n");
	}


	string message;
	if (cmd == "PUT" || cmd == "MKD") {
		message = "PUT /"+fd;
		if (cmd == "PUT") {
			message += "?type=file HTTP/1.1\r\n";
		} else {
			message += "?type=folder HTTP/1.1\r\n";
		}
	} else if (cmd == "DEL" || cmd == "RMD") {
		message = "DELETE /"+fd;
		if (cmd == "DEL") {
			message += "?type=file HTTP/1.1\r\n";
		} else {
			message += "?type=folder HTTP/1.1\r\n";
		}
	} else if (cmd == "LST" || cmd == "GET") {
		message = "GET /"+fd;
		if (cmd == "GET") {
			message += "?type=file HTTP/1.1\r\n";
		} else {
			message += "?type=folder HTTP/1.1\r\n";
		}
	}

	if (fd.find("userpw") != std::string::npos) {
		throwException("Error: You can't download user passwords.");
	}

	// length of message - this counts only binary conent

	cout << "Password: (won't be hidden) ";

	string password;
	cin >> password;

	int length = 0;
	string fileBin;
	if (cmd == "PUT") {
		fileBin = loadFile(localFile);
		length = fileBin.size();
	}

	string len;
	ostringstream convert;
	convert << length;
	len = convert.str();

	message += "Host: "+hostName+"\r\n";
	message += "Date: "+getCurrDate()+"\r\n";
	message += "Accept: application/json\r\n";
	message += "Accept-Encoding: identity\r\n";
	message += "Content-Type: application/octet-stream\r\n";
	message += "Content-Length: "+len+"\r\n";
	message += "Authorization: Basic "+password;

	message += "\r\n\r\n";
	if (cmd == "PUT") {
		message += fileBin;
	}

	struct sockaddr_in serveraddr;
	struct hostent *hostent;

	if ((hostent = gethostbyname(hostName.c_str())) == NULL) {
		throwException("Error: Couldn't resolve name.\n");
	} else {
		memset((char *) &serveraddr, '\0', sizeof(serveraddr));
		memcpy(&serveraddr.sin_addr, hostent->h_addr, hostent->h_length);
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	//bcopy((char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int client_socket;
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		throwException("Error: Could not open client socket.\n");
	}

	if ((connect(client_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
		throwException("Error: Couldn't connect to server.\n");
	}

	if (message.size() > 1024) {
		throwException("Error: You can't send files bigger than 1024 chars. Client is limited with the message sizes.");
	}

	cout << message.size() << endl;

	send(client_socket, message.c_str(), 1024, 0);

	char response[1024];
	string rsp = "";
	int rcv;
	if ((rcv = recv(client_socket, response, 1025, 0)) > 0) {

		rsp += response;

		string cl = returnSubstring(returnSubstring(response,"Content-Length: ", true),"\r\n", false);
		int rcvLen = atoi(cl.c_str());

		if (rcvLen > 900) {
			rcvLen -= returnSubstring(response, "\r\n\r\n", true).length();
			while (rcvLen > 0) {
				memset(&response, '\0', sizeof(response));
				recv(client_socket, response, 1025, 0);
				rsp += response;
				rcvLen -= 1024;
			}
		}




		string msg = returnSubstring(rsp, "\r\n\r\n", true);

		if (strcmp(argv[1], "get") == 0) {
			string file = returnSubstring(msg, "\r\n", true);
			msg = returnSubstring(msg, "\r\n", false);
			ofstream strfile;
			if (argc == 3) {
				strfile.open(fd, ios::out | ios::binary);
			} else {
				strfile.open(localFile, ios::out | ios::binary);
			}

			strfile.write(file.c_str(), file.size());
			strfile.close();
		}

		if (msg.find("Success.") == std::string::npos) {
			close(client_socket);
			throwException(("Error: "+msg).c_str());
		} else {
			cout << "Info: ";
			cout << msg << endl;
		}
	} else {
		throwException("Error: Couldn't read data from server correctly.");
	}

	if (close(client_socket) != 0) {
		throwException("Error: Could not close socket.\n");
	}


	return 0;
}