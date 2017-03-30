#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <dirent.h>

#ifdef _WIN32
	#include <Winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <sstream>
#include <iostream>
#include <vector>
#include <utility>
#include <ctime>

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
 * Function checks if the folder is regular folder, it is typically called after itemExists(), it shouldn't be called without this function
 *
 * @param const char *fd name of the folder
 * @return 1 if folder exists and is regular folder, 0 otherwise
 */
bool isDirectory(const char *folder) {
	struct stat sb;

	return ((stat(folder, &sb) == 0) && S_ISDIR(sb.st_mode));
}


/*
 * Function checks if the file is regular file, it is typically called after itemExists(), it shouldn't be called without this function
 *
 * @param const char *fd name of the file
 * @return 1 if file exists and is regular file, 0 otherwise
 */
bool isFile(const char *file) {
	struct stat sb;

	return ((stat(file, &sb) == 0) && S_ISREG(sb.st_mode));
}


/*
 * Function checks for item existence
 *
 * @param const char *fd name of the file
 * @return 1 if file exists, 0 otherwise
 */
bool itemExists(const char *fd) {

	struct stat sb;

	return (stat(fd, &sb) == 0);

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
		dateTime = "Date could not be resolved";
	}

	return dateTime;
}

/*
 * Functions generates message for the client
 *
 * @param int statusCode HTTP Status
 * @param string message Special message from server
 * @param type Type of output for client
 * @param length of response message, typically 0 + length of message given
 *
 * @return string response
 */
string generateMessage(int statusCode, string message, string type, int messageLength) {

	ostringstream status;
	status << statusCode;
	string text;

	if (statusCode == 200) {
		text = " OK";
	} else if (statusCode == 400) {
		text = " Bad Request";
	} else if (statusCode == 404) {
		text = " Not Found";
	} else {
		text = " ";
	}

	ostringstream len;
	len << messageLength;

	string response = "HTTP/1.1 "+status.str()+text+"\r\n";
	response += "Date: "+getCurrDate()+"\r\n";
	response += "Content-Type: "+type+"\r\n";
	response += "Content-Length: "+len.str()+"\r\n";
	response += "Content-Encoding: identity\r\n\r\n";
	response += message;

	return response;

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
 * Functions parses the message and fills up the header array with correct values
 *
 * @param string message from client
 * @param string *header array containing parsed http header
 */
string* parseReceivedMessageHeader(string message) {

	bool headerFlags[12] = {
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false
	};

	string *headerItems = new string[12];

	if (message.find("\r\n\r\n") != string::npos) {


		string header = returnSubstring(message, "\r\n\r\n", false);
		header += "\r\n";


		string firstLine = returnSubstring(message, "\r\n", false);
		header = returnSubstring(header, firstLine, true); // remove firstline from header

		headerItems[0] = returnSubstring(firstLine, " ", false); // command
		headerFlags[0] = true;

		firstLine = returnSubstring(firstLine, " /", true);
		headerItems[1] = returnSubstring(firstLine, "/", false); //user
		headerFlags[1] = true;

		firstLine = returnSubstring(firstLine, "/", true);

		headerItems[2] = returnSubstring(firstLine, "?type=", false); // folder/file
		headerFlags[2] = true;

		firstLine = returnSubstring(firstLine, "?type=", true);

		headerItems[3] = returnSubstring(firstLine, " ", false); // type
		headerFlags[3] = true;

		firstLine = returnSubstring(firstLine, " ", true);

		headerItems[4] = firstLine; // http protocol
		headerFlags[4] = true;

		headerItems[5] = returnSubstring(returnSubstring(header, "Host: ", true), "\r\n", false);
		headerFlags[5] = true;
		headerItems[6] = returnSubstring(returnSubstring(header, "Date: ", true), "\r\n", false);
		headerFlags[6] = true;
		headerItems[7] = returnSubstring(returnSubstring(header, "Accept: ", true), "\r\n", false);
		headerFlags[7] = true;
		headerItems[8] = returnSubstring(returnSubstring(header, "Accept-Encoding: ", true), "\r\n", false);
		headerFlags[8] = true;
		headerItems[9] = returnSubstring(returnSubstring(header, "Content-Type: ", true), "\r\n", false);
		headerFlags[9] = true;
		headerItems[10] = returnSubstring(returnSubstring(header, "Content-Length: ", true), "\r\n", false);
		headerFlags[10] = true;
		headerItems[11] = returnSubstring(returnSubstring(header, "Authorization: Basic ", true), "\r\n", false);
		headerFlags[11] = true;
	}

	return headerItems;
}

/*
 * Function checks for header validity according to RESTapi, more info at http://restapitutorial.com
 *
 * @param string *header array containing parsed http header
 * @return int statusCode if header is valid, 200 is returned, otherwise 400 is returned
 */
int checkHeaderValidity(string *header) {

	int statusCode = 200;

	for (int i = 0; i < sizeof(header); i++) {
		if (header[i] == "") {
			if (i == 2) {
				continue;
			}
			statusCode = 400;
			break;
		}
	}

	if (header[0] != "PUT" && header[0] != "GET" && header[0] != "DELETE") {
		statusCode = 400;
	} else if (header[4] != "HTTP/1.1") {
		statusCode = 400;
	} else if (header[3] != "file" && header[3] != "folder") {
		statusCode = 400;
	}

	return statusCode;
}

/*
 * Function is given a header and checks for correct operation which should be done
 *
 * @param string *header array containing parsed http header
 * @return -1 if error occured, 0 - 2 for file actions, 3 - 5 for folder actions
 */
int checkWhichFunction(string *header) {

	int code = -1;// 0 - 2 reserved for file actions, 3 - 5 for folder actions, -1 error

	if (header[3] == "file") {
		// put get, del
		if (header[0] == "PUT") {
			code = 0;
		} else if (header[0] == "GET") {
			code = 1;
		} else if (header[0] == "DELETE"){
			code = 2;
		} else {
			code = -1;
		}
	} else if (header[3] == "folder") {
		// put, get, del
		if (header[0] == "PUT") {
			code = 3;
		} else if (header[0] == "GET") {
			code = 4;
		} else if (header[0] == "DELETE"){
			code = 5;
		} else {
			code = -1;
		}
	}

	return code;
}

/*
 * Function checks if user given in message is existent
 *
 * @param string user name of the user
 * @return 0 if user does not exist, else 1 is returned
 */
bool checkForUser(string user) {
	if (!isDirectory(user.c_str())) {
		return 0;
	}
	return 1;
}


/*
 * Function creates the file in the given file and writes binary code in it
 *
 * @param string file path to the file
 * @param string bin binary representation of file
 * @param int length
 * @return 0 if the file was created, 1 if file already exists, 2 if directory given for file was not found
 *
 */
int putFile(string file, string bin, int length) {

	string folder = file.substr(0,file.find_last_of("/"));

	if (isDirectory(folder.c_str())) {
		if (itemExists(file.c_str()) && isFile(file.c_str())) {
			return 1;
		} else {
			ofstream strFile;
			strFile.open(file, ios::out | ios::binary | ios::app);
			strFile.write(bin.c_str(), bin.size());
			strFile.close();
			return 0;
		}
	} else {
		return 2;
	}

	return 0;
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
		return "";
	}

}

/*
 *
 */
int getFile() {
	return 0;
}

/*
 * Function deletes the file given in the argument
 *
 * @param string file path to the file
 * @returns 0 if file was successfully deleted, 1 if file couldn't be deleted, 2 if it is not a file, 3 if file was not found
 */
int delFile(string file) {

	if (itemExists(file.c_str())) {
		if (isFile(file.c_str())) {
			if (unlink(file.c_str()) == 0) {
				return 0;
			} else {
				return 1;
			}
		} else {
			return 2;
		}
	} else {
		return 3;
	}
}


/*
 * Function creates the directory given in the argument
 *
 * @param string folder path to the folder that should be created
 * @returns 0 if dir was successfully created, 1 if directory couldn't be created, 2 if dir already exists
 */
int createDir(string folder) {

	if (itemExists(folder.c_str())) {
		if (isDirectory(folder.c_str())) {
			return 2;
		} else {
			if (mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
				return 0;
			} else {
				return 1;
			}
		}
	} else {
		if (mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
			return 0;
		} else {
			return 1;
		}
	}
}

/*
 * Folder works as a "ls" from bash, returns string of all files in the folder
 *
 * @param string folder path to the folder
 * @return string content of the folder
 */
string getDirContent(string folder) {

	DIR *dir;
	struct dirent *ent;

	string content = "";

	if ((dir = opendir(folder.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
				continue;
			}
			content += ent->d_name;
			content += "\r\n";
		}
	}

	return content;
}

/*
 * Functions deletes directory given in the argument
 *
 * @param string folder path to the folder to be removed
 * @return 1 if directory does not exist, 2 if dir is not dir, 3 if dir contains files, 0 if everything went well
 */
int delDir(string folder) {

	if (itemExists(folder.c_str())) {
		if (isDirectory(folder.c_str())) {
			if (rmdir(folder.c_str()) == 0) {
				return 0;
			} else {
				return 3;
			}
		} else {
			return 2;
		}
	} else {
		return 1;
	}
}

/*
 * Functions sends response according to the size of response, if the response is bigger than 1 packet, it is split into more and send peridically until everything is sent
 *
 * @param int comm_socket socket that should be used for communication
 * @param string response response that is about to be sent
 * @param int code code is curretly not used, represents code from whichToDo() function
 */
void sendResponse(int comm_socket, string response, int code) {

	char rsp[1025];
	int respLen = response.length();
	string subMessage;
	unsigned int index = 0;

	memset(&rsp, '\0', sizeof(rsp));
	if (respLen > 1024) {
		while (respLen > 1024) {
			memset(&rsp, '\0', sizeof(rsp));
			subMessage = response.substr(1024*index);
			subMessage = subMessage.substr(0, 1024);
			subMessage.erase(subMessage.end(), subMessage.end());
			memcpy(&rsp, subMessage.c_str(), 1025);

			if (send(comm_socket, rsp, sizeof(rsp), 0) < 0) {
				// err ?
			}
		   	index++;
			respLen -= 1024;
		}
		memset(&rsp, '\0', sizeof(rsp));
		subMessage = response.substr(1024*(index));
		subMessage.erase(subMessage.end(), subMessage.end());
		memcpy(&rsp, subMessage.c_str(), 1025);
		if (send(comm_socket, rsp, sizeof(rsp), 0) < 0) {
			// err ?
		}

	} else {
		memcpy(&rsp, response.c_str(), response.length());
		send(comm_socket, rsp, sizeof(rsp), 0);
	}

}

/*
 * Function looks for user password in the predeterminated file
 *
 * @param string file file that is being searched
 * @param string user user whose password is looked for
 * @return string password returns password if everything went well, otherwise returns blank string
 */
string findUserPw(string file, string user) {

	string userlist = "";
	string line;

	ifstream users (file.c_str());
	if (users.is_open()) {
		while (getline(users, line)) {
			if (line.find(user) != std::string::npos && line.find(user) == 0) {
				return returnSubstring(line, " ", true);
			}
		}
	} else {
		throwException("Error: Couldn't open userpw file.");
	}

	users.close();

	return "";
}

/*
 * Function checks if two passwords are equal
 *
 * @param string pw1 first password
 * @param string pw2 second password
 * @return 1 if passwords are equal, otherwise 0 is returned
 */
bool checkPassword(string pw1, string pw2) {
	return (pw1 == pw2);
}


int main(int argc, char *argv[]) {

	if ((argc == 2) || (argc == 4) || (argc > 5)) {
		throwException("Error: Wrong amount of arguments.\n");
	} else {
		bool pTag = false;
		bool rTag = false;
		int port = 6677;
		char cwd[1024];
		char errmessage[1024];
		char receivedMessage[1024];
		char rsp[1024];
		memset((char *) &cwd, '\0', sizeof(cwd));

		if (argc < 4 && argc > 1) {
			if (strcmp(argv[1], "-p") == 0) {
				port = atoi(argv[2]);
				pTag = true;
			} else if (strcmp(argv[1], "-r") == 0) {
				strcpy(cwd, argv[2]);
				if(!isDirectory(cwd)) {
					strcpy(errmessage, "Error: Directory ");
					strcat(errmessage,strcat((char *)cwd, " does not exist.\n"));
					throwException(errmessage);
				};
				rTag = true;
			}
		} else if (argc > 4) {

			if (strcmp(argv[1], "-p") == 0) {
				port = atoi(argv[2]);
				pTag = true;
			} else if (strcmp(argv[1], "-r") == 0) {
				strcpy(cwd, argv[2]);
				if(!isDirectory(cwd)) {
					strcpy(errmessage, "Error: Directory ");
					strcat(errmessage,strcat((char *)cwd, " does not exist.\n"));
					throwException(errmessage);
				};
				rTag = true;
			}

			if (strcmp(argv[3], "-p") == 0) {
				if (pTag) {
					throwException("Bad arguments.\n");
				}
				port = atoi(argv[4]);
				pTag = true;
			} else if (strcmp(argv[3], "-r") == 0) {
				if (rTag) {
					throwException("Bad arguments.\n");
				}
				strcpy(cwd, argv[4]);
				if(!isDirectory(cwd)) {
					strcpy(errmessage, "Error: Directory ");
					strcat(errmessage,strcat((char *)cwd, " does not exist.\n"));
					throwException(errmessage);
				};

				rTag = true;
			}
		}

		if (port < 1024 || port > 65535) {
			throwException("Error: Wrong port range.\n");
		}

		if (!rTag) {
			if (strcpy(cwd,(getcwd(cwd,sizeof(cwd)))) == NULL) {
				strcpy(errmessage, "Error: Cannot get CWD ");
				strcat(errmessage,strcat((char *)cwd, ".\n"));
				throwException(errmessage);
			}
		} else {
			if (chdir(cwd) < 0) {
				strcpy(errmessage, "Error: Cannot change CWD to ");
				strcat(errmessage,strcat((char *)cwd, ".\n"));
				throwException(errmessage);
			}
		}

		if (!isFile("userpw")) {
			throwException("Error: userpw file does not exist.");
		}


		// we will create and open a socket
		int server_socket;

		if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
			throwException("Error: Could not open server socket.\n");
		}


		int optval = 1;
		setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
				   (const void *)&optval , sizeof(int));

		struct sockaddr_in clientaddr;
		struct sockaddr_in serveraddr;
		memset((char *) &serveraddr, '\0', sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons((unsigned short) port);
		//bcopy((char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length);
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

		int rc;
		if ((rc = bind(server_socket, (struct sockaddr*)&serveraddr, sizeof(serveraddr))) < 0) {
			throwException("Error: Could not bind to port.\n");
		}


		if ((listen(server_socket, 1)) < 0) {
			throwException("Error: Could not start listening on port.\n");
		}
		printf("DEBUG INFO\n");
		printf("Port: %d\nCWD: %s\n", port, cwd);

		printf("Server is online! It will be now waiting for request.\n");

		socklen_t clientlen = sizeof(clientaddr);
		while(1) {
			int comm_socket;
			int recieved = 0;

			if ((comm_socket = accept(server_socket, (struct sockaddr*)&clientaddr, &clientlen)) > 0) {

				for(;;) {
					if ((recieved = recv(comm_socket, receivedMessage, 1024, 0)) <= 0) {
						break;
					} else {
						memset((char *) &rsp, '\0', sizeof(rsp));
						string stringMessage(receivedMessage);
						string *header = parseReceivedMessageHeader(receivedMessage);
						string response = "";
						int statusCode = checkHeaderValidity(header);
						if (statusCode != 200) {
							// samfin err
							response = generateMessage(statusCode, "Unknown error", "text/plain", 0);

							memcpy(&rsp, response.c_str(), response.length());
							send(comm_socket, rsp, sizeof(rsp), 0);
						} else {
							if (!checkForUser(header[1])) {
								statusCode = 404;
								response = generateMessage(statusCode, "User Account Not Found", "text/plain", 0);
								memcpy(&rsp, response.c_str(), response.length());
								send(comm_socket, rsp, sizeof(rsp), 0);
							} else if (!checkPassword(findUserPw("userpw", header[1]), header[11])) {
								statusCode = 401;
								response = generateMessage(statusCode, "Unauthorized.", "text/plain", 0);
								sendResponse(comm_socket, response, -1);
							} else {
								int rst;
								int whatToDo = checkWhichFunction(header);
								if (whatToDo < 3) { // file operations
									if (whatToDo == 0) {

										string binFile = returnSubstring(stringMessage,"\r\n\r\n", true);
										int len = atoi(header[10].c_str());
										if ((rst = putFile(header[1] + "/" + header[2], binFile, len)) == 0) {
											statusCode = 200;
											response = generateMessage(statusCode, "Success.\r\n", "text/plain", 0);
										} else if (rst == 1) {
											statusCode = 400;
											response = generateMessage(statusCode, "Already exists.", "text/plain", 0);
										} else if (rst == 2) {
											statusCode = 404;
											response = generateMessage(statusCode, "Directory not found.",
																	   "text/plain", 0);
										} else {
											statusCode = 404;
											response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
										}
										sendResponse(comm_socket, response, 1);
									} else if (whatToDo == 1) {
										if (itemExists((header[1]+"/"+header[2]).c_str())) {
											if (isFile((header[1]+"/"+header[2]).c_str())) {
												string fileBin;
												if ((fileBin = loadFile(header[1]+"/"+header[2])) == "") {
													statusCode = 400;
													response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
												} else {
													statusCode = 200;
													response = generateMessage(statusCode, "Success.\r\n", "application/octet-stream", fileBin.size());
													response += fileBin;
												}
											} else {
												statusCode = 400;
												response = generateMessage(statusCode, "Not a file.", "text/plain", 0);
											}
										} else {
											statusCode = 404;
											response = generateMessage(statusCode, "File not found.", "text/plain", 0);
										}
										sendResponse(comm_socket, response, 1);
									} else if (whatToDo == 2) {
											if ((rst = delFile(header[1] + "/" + header[2])) == 0) {
												statusCode = 200;
												response = generateMessage(statusCode, "Success.\r\n", "text/plain", 0);
											} else if (rst == 1) {
												statusCode = 400;
												response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
											} else if (rst == 2) {
												statusCode = 400;
												response = generateMessage(statusCode, "Not a file.", "text/plain", 0);
											} else if (rst == 3) {
												statusCode = 404;
												response = generateMessage(statusCode, "File not found.", "text/plain", 0);
											} else {
												statusCode = 400;
												response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
											}
											memcpy(&rsp, response.c_str(), response.length());
											sendResponse(comm_socket, response, 2);
									} else {
										statusCode = 400;
										response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
									}
								} else { // folder operations
									if (whatToDo == 3) {
										if ((rst = createDir(header[1] + "/" + header[2])) == 0) {
											statusCode = 200;
											response = generateMessage(statusCode, "Success.\r\n", "text/plain", 0);
										} else if (rst == 1) {
											statusCode = 400;
											response = generateMessage(statusCode, "Unkwnown error.", "text/plain", 0);
										} else if (rst == 2) {
											statusCode = 400;
											response = generateMessage(statusCode, "Already exists.", "text/plain", 0);
										} else {
											statusCode = 400;
											response = generateMessage(statusCode, "Unkwnown error.", "text/plain", 0);
										}
										memcpy(&rsp, response.c_str(), response.length());
										send(comm_socket, rsp, sizeof(rsp), 0);
										break;
									} else if (whatToDo == 4) {
										// lst folder
										if (itemExists((header[1] + "/" + header[2]).c_str())) {
											if (isDirectory((header[1] + "/" + header[2]).c_str())) {
												string directoryContent = getDirContent(
														header[1] + "/" + header[2]);
												statusCode = 200;
												response = generateMessage(statusCode, "Success.\r\n", "text/plain", directoryContent.length());
												if (directoryContent != "") {
													response += "\r\n" + directoryContent;
												}
											} else {
												statusCode = 400;
												response = generateMessage(statusCode, "Not a directory.",
																		   "text/plain", 0);
											}
										} else {
											statusCode = 404;
											response = generateMessage(statusCode, "Directory not found.",
																	   "text/plain", 0);
										}

										sendResponse(comm_socket, response, 4);

									} else if (whatToDo == 5) {
											// delete folder
											if (header[1]+"/"+header[2] != header[1]) {
												if ((rst = delDir(header[1] + "/" + header[2])) == 0) {
													statusCode = 200;
													response = generateMessage(statusCode, "Success.\r\n", "text/plain", 0);
												} else if (rst == 1) {
													statusCode = 404;
													response = generateMessage(statusCode, "Directory not found.",
																			   "text/plain", 0);
												} else if (rst == 2) {
													statusCode = 400;
													response = generateMessage(statusCode, "Not a directory.",
																			   "text/plain", 0);
												} else if (rst == 3) {
													statusCode = 400;
													response = generateMessage(statusCode, "Directory not empty.",
																			   "text/plain", 0);

												} else {
													statusCode = 400;
													response = generateMessage(statusCode, "Unknown error.",
																			   "text/plain", 0);
												}
											} else {
												statusCode = 400;
												response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
											}
											memcpy(&rsp, response.c_str(), response.length());
											send(comm_socket, rsp, sizeof(rsp), 0);
									} else {
										statusCode = 400;
										response = generateMessage(statusCode, "Unknown error.", "text/plain", 0);
									}
								}
							}
						}

						delete [] header;

						memset(&receivedMessage, '\0', sizeof(receivedMessage));
					}
				}
			} else {
				throwException("Error: Could not open client socket.\n");
			}
		}

		if (close(server_socket) != 0) {
			throwException("Error: Could not close socket.\n");
		}
	}

	return 0;
}