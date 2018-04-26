// Definitions, function declarations, and variables for CLI
#define CLI_BUF_SIZE 128
#define ARG_BUF_SIZE 128
#define ARG_MAX_NUM 4

int cmd_help();
int cmd_show();
int cmd_set();

// Function pointers corresponding to each command
int (*commands_func[3])() = {
	&cmd_help,
	&cmd_show,
	&cmd_set
};

char cli_line[CLI_BUF_SIZE];
char args[ARG_MAX_NUM][ARG_BUF_SIZE];

// Variables shared between CLI and other stuff
char* ssid;
char* password;
char* serverIp; // pass into udpIP 
int* port;
int* period; // call when delaying sense distance task

// Character arrays of each set of CLI commands
const char *commands_str[] = {
	"HELP",
	"SHOW",
	"SET"
};
int num_commands = sizeof(commands_str) / sizeof(char *);

const char *param_args[] = {
	"WIFI",
	"SERVER",
	"SENSOR"
};
int num_params = sizeof(param_args) / sizeof(char *);

const char *wifi_args[] = {
	"SSID",
	"PASS"
};
const char *server_args[] = {
	"ADDRESS",
	"PORT"
};
const char *sensor_args[] = {
	"PERIOD",
};

void run_cli() {
	String line_string;
	// Continuously search for serial input
	for(;;) {
		while(!Serial.available());
		if(Serial.available()) {
			line_string = Serial.readStringUntil('\n');
			// When received input, parse and execute command
			if(line_string.length() < CLI_BUF_SIZE) {
				line_string.toCharArray(cli_line, CLI_BUF_SIZE);
				Serial.println(line_string);
				parseLine();
			} else {
				Serial.println("Command entered is too long.");
			}
		}
	// Reset variables for next reading of serial input
	memset(cli_line, 0, CLI_BUF_SIZE);
	memset(args, 0, sizeof(args[0][0]) * ARG_MAX_NUM * ARG_BUF_SIZE);
	}
}

int parseLine() {
	char *argument;
	int counter = 0;
	argument = strtok(cli_line, " ");

	// Loop through arguments separated by space
	while ((argument != NULL)) {
		if(counter < ARG_MAX_NUM) {
			if(strlen(argument) < ARG_BUF_SIZE) {
				// Add argument to array
				strcpy(args[counter], argument);
				argument = strtok(NULL, " ");
				counter++;
			} else {
				Serial.println("Command entered is too long.");
				break;
			}
		} else {
			break;
		}
	}
	// Find base command function matching input
	for(int i=0; i < num_commands; i++) {
		if(strcmp(args[0], commands_str[i]) == 0) {
			return (*commands_func[i])();
		}
	}
	Serial.println("Invalid command. Type HELP for more information.");
	return 0;
}

// HELP functions to provide clarification about commands
int cmd_help() {
	if(args[1] == NULL) {
		Serial.println("Please enter a valid command. See HELP options for more information.");
	 	help_help();
	} else if(strcmp(args[1], commands_str[0]) == 0) {
		help_help();
	}
	else if(strcmp(args[1], commands_str[1]) == 0) {
		help_show();
	}
	else if(strcmp(args[1], commands_str[2]) == 0) {
		help_set();
	}
	else{
		Serial.println("Please enter a valid command. See HELP options for more information.");
		help_help();
	}
}

void help_help() {
	Serial.println("[HELP] Command: HELP\n[HELP] Description: displays the different CLI options.");
	Serial.println("[HELP] Commands: ");
	for(int i=0; i < num_commands; i++) {
		Serial.print(" - ");
		Serial.println(commands_str[i]);
	}
	Serial.println("[HELP] Usage: Type HELP <command> for details on that specific option (ex. \"HELP SHOW\").");
}

void help_show() {
	Serial.println("[HELP] Command: SHOW");
	Serial.println("[HELP] Description: Displays the data values currently set for a given parameter.");
	print_params();
	Serial.println("[HELP] Usage: Type SHOW <param> to see all data for that parameter (ex. \"SHOW WIFI\"), or SHOW <param> <variable> to view a specific variable (ex. \"SHOW WIFI SSID\").");
}

void help_set() {
	Serial.println("[HELP] Command: SET");
	Serial.println("[HELP] Description: Allows you to alter a variable in the program.");
	print_params();
	Serial.println("[HELP] Usage: Type SET <param> <variable> <value> to change that variable (ex. \"SET WIFI SSID wifi-network-name\").");
}

void print_params() {
	Serial.println("[HELP] Parameters:");
	for(int i=0; i < num_params; i++) {
		Serial.print(" - ");
		Serial.println(param_args[i]);
		Serial.println("	Variables:");
		if(strcmp(param_args[i], "WIFI") == 0) {
			Serial.println("	 - SSID: Wifi connection name");
			Serial.println("	 - PASS: Wifi password");
		} else if(strcmp(param_args[i], "SERVER") == 0) {
			Serial.println("	 - ADDRESS: Server IP address");
			Serial.println("	 - PORT: Server port");
		} else if(strcmp(param_args[i], "SENSOR") == 0) {
			Serial.println("	 - PERIOD: Sensor period between distances");
		}
	}
}

// SHOW functions to display currently set values
void show_wifi() {
	if(args[2] == NULL) {
		Serial.print("[SHOW] Wifi SSID: ");
		Serial.println(ssid);
		Serial.print("[SHOW] Wifi Password: ");
		Serial.println(password);
	} else if(strcmp(args[2], wifi_args[0]) == 0) {
		Serial.print("[SHOW] Wifi SSID: ");
		Serial.println(ssid);
	} else if(strcmp(args[2], wifi_args[1]) == 0) {
		Serial.print("[SHOW] Wifi Password: ");
		Serial.println(password);
	} else {
		Serial.println("[SHOW] You must specify either a valid variable or none. Type HELP SHOW for more information.");
	}
}

void show_server() {
	if(args[2] == NULL) {
		Serial.print("[SHOW] Server IP Address: ");
		Serial.println(serverIp);
		Serial.print("[SHOW] Server Port: ");
		Serial.println(*port);
	} else if(strcmp(args[2], wifi_args[0]) == 0) {
		Serial.print("[SHOW] Server IP Address: ");
		Serial.println(serverIp);
	} else if(strcmp(args[2], wifi_args[1]) == 0) {
		Serial.print("[SHOW] Server Port: ");
		Serial.println(*port);
	} else {
		Serial.println("[SHOW] You must specify either a valid variable or none. Type HELP SHOW for more information.");
	}
}

void show_sensor() {
	if(args[2] == NULL) {
		Serial.print("[SHOW] Sensor Period: ");
		Serial.println(*period);
	} else if(strcmp(args[2], wifi_args[0]) == 0) {
		Serial.print("[SHOW] Sensor Period: ");
		Serial.println(*period);
	} else {
		Serial.println("[SHOW] You must specify either a valid variable or none. Type HELP SHOW for more information.");
	}
}

int cmd_show() {
	if(args[1] == NULL) {
		Serial.println("[SHOW] You must specify a valid parameter. Type HELP SHOW for more information.");
	} else if(strcmp(args[1], param_args[0]) == 0) {
		show_wifi();
	} else if(strcmp(args[1], param_args[1]) == 0) {
		show_server();
	} else if(strcmp(args[1], param_args[2]) == 0) {
		show_sensor();
	} else {
		Serial.println("[SHOW] You must specify a valid parameter. Type HELP SHOW for more information.");
	}
}

// SET function to alter the specified parameter
void set_wifi() {
	if(args[2] == NULL) {
		Serial.println("[SET] You must specify a valid variable. Type HELP SET for more information.");
	} else if(strcmp(args[2], wifi_args[0]) == 0) {
		if(args[3] == NULL) {
			Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
		} else {
			strcpy(ssid, args[3]);
		}
	} else if(strcmp(args[2], wifi_args[1]) == 0) {
		if(args[3] == NULL) {
			Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
		} else {
			strcpy(password, args[3]);
		}
	} else {
		Serial.println("[SET] You must specify a valid variable. Type HELP SET for more information.");
	}
}

void set_server() {
	if(args[2] == NULL) {
		Serial.println("[SET] You must specify a valid variable. Type HELP SET for more information.");
	} else if(strcmp(args[2], server_args[0]) == 0) {
		if(args[3] == NULL) {
			Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
		} else {
			strcpy(serverIp, args[3]);
		}
	} else if(strcmp(args[2], wifi_args[1]) == 0) {
		if(args[3] == NULL) {
			Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
		} else {
			memcpy(port, args[3], strlen(args[3] + 1));
		}
	} else {
		Serial.println("[SET] You must specify a valid variable. Type HELP SET for more information.");
	}
}

void set_sensor() {
	if(args[2] == NULL) {
		Serial.println("[SET] You must specify a valid variable. Type HELP SET for more information.");
	} else if(strcmp(args[2], sensor_args[0]) == 0) {
		if(args[3] == NULL) {
			Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
		} else {
			memcpy(period, args[3], strlen(args[3]) + 1);
		}
	} else {
		Serial.println("[SET] You must specify a valid variable. Type HELP SET for more information.");
	}
}

int cmd_set() {
	if(args[1] == NULL) {
		Serial.println("[SET] You must specify a valid parameter. Type HELP SET for more information");
	} else if(strcmp(args[1], param_args[0]) == 0) {
		set_wifi();
		// Call function to reconnect to wifi 
	} else if(strcmp(args[1], param_args[1]) == 0) {
		set_server();
		
	} else if(strcmp(args[1], param_args[2]) == 0) {
		set_sensor();
	} else {
		Serial.println("[SET] You must specify a valid parameter. Type HELP SET for more information");
	}
}

// The necessary things that probably already exist, just be sure Serial.begin is in there
void setup()
{
	Serial.begin(9600);
	xTaskCreate(
	run_cli,
	"cli",
	4000,
	NULL,
	4,
	NULL
);
}

void loop()
{

}