// Definitions, function declarations, and variables for CLI
#define CLI_BUF_SIZE 128
#define ARG_BUF_SIZE 128
#define ARG_MAX_NUM 3

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
char ssid[ARG_BUF_SIZE];
char password[ARG_BUF_SIZE];
char ipaddress[ARG_BUF_SIZE]; // pass into udpIP
char port[ARG_BUF_SIZE];
char period[ARG_BUF_SIZE]; // call when delaying sense distance task
char buffer[ARG_BUF_SIZE];

// Character arrays of each set of CLI commands
const char *commands_str[] = {
    "HELP",
    "SHOW",
    "SET"
};
int num_commands = sizeof(commands_str) / sizeof(char *);

const char *param_args[] = {
    "--ssid",
    "--password",
    "--ipaddress",
    "--port",
    "--period",
    "--buffer"
};
int num_params = sizeof(param_args) / sizeof(char *);

void run_cli(void *pvParameters) {
    EEPROM.put(CONFIG_ADDR, configVars);
    String line_string;
    // Continuously search for serial input
    for (;;) {
        while (!Serial.available());
        if (Serial.available()) {
            line_string = Serial.readStringUntil('\n');
            // When received input, parse and execute command
            if (line_string.length() < CLI_BUF_SIZE) {
                line_string.toCharArray(cli_line, CLI_BUF_SIZE);
                Serial.println(line_string);
                parseLine();
            } else {
                Serial.println("Command entered is too long.");
            }
        }
        // Reset variables for next reading of serial input
        memset(cli_line, 0, CLI_BUF_SIZE);
        memset(args, 0, sizeof(args[ARG_MAX_NUM][ARG_BUF_SIZE]));
    }
}

int parseLine() {
    char *argument;
    int counter = 0;
    argument = strtok(cli_line, " ");

    // Loop through arguments separated by space
    while ((argument != NULL)) {
        if (counter < ARG_MAX_NUM) {
            if (strlen(argument) < ARG_BUF_SIZE) {
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
    for (int i = 0; i < num_commands; i++) {
        if (strcmp(args[0], commands_str[i]) == 0) {
            return (*commands_func[i])();
        }
    }
    Serial.println("Invalid command. Type HELP for more information.");
    return 0;
}

// HELP functions to provide clarification about commands
void help_help() {
    Serial.println("[HELP] Command: HELP\n[HELP] Description: displays the different CLI options.");
    Serial.println("[HELP] Commands: ");
    for (int i = 0; i < num_commands; i++) {
        Serial.print(" - ");
        Serial.println(commands_str[i]);
    }
    Serial.println("[HELP] Usage: Type HELP <command> for details on that specific option (ex. \"HELP SHOW\").");
}

void help_show() {
    Serial.println("[HELP] Command: SHOW");
    Serial.println("[HELP] Description: Displays the data values currently set for a given parameter.");
    print_params();
    Serial.println("[HELP] Usage: Type SHOW <param> to see that variable's value (ex. \"SHOW  --ssid\").");
}

void help_set() {
    Serial.println("[HELP] Command: SET");
    Serial.println("[HELP] Description: Allows you to alter a variable in the program.");
    print_params();
    Serial.println("[HELP] Usage: Type SET <param> <value> to change that variable (ex. \"SET --ssid wifi-network-name\").");
}

int cmd_help() {
    if (args[1] == NULL) {
        Serial.println("Please enter a valid command. See HELP options for more information.");
        help_help();
    } else if (strcmp(args[1], commands_str[0]) == 0) {
        help_help();
    } else if (strcmp(args[1], commands_str[1]) == 0) {
        help_show();
    } else if (strcmp(args[1], commands_str[2]) == 0) {
        help_set();
    } else {
        Serial.println("Please enter a valid command. See HELP options for more information.");
        help_help();
    }
}

void print_params() {
    Serial.println("[HELP] Parameters:");
    for (int i = 0; i < num_params; i++) {
        Serial.print("  ");
        Serial.println(param_args[i]);
    }
}

// SHOW function to display currently set values
int cmd_show() {
    if (args[1] == NULL) {
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(password);
        Serial.print("IP Address: ");
        Serial.println(ipaddress);
        Serial.print("Port: ");
        Serial.println(port);
        Serial.print("Period: ");
        Serial.println(period);
        Serial.print("Buffer: ");
        Serial.println(buffer);
    } else if (strcmp(args[1], param_args[0]) == 0) {
        Serial.print("SSID: ");
        Serial.println(ssid);
    } else if (strcmp(args[1], param_args[1]) == 0) {
        Serial.print("Password: ");
        Serial.println(password);
    } else if (strcmp(args[1], param_args[2]) == 0) {
        Serial.print("IP Address: ");
        Serial.println(ipaddress);
    } else if (strcmp(args[1], param_args[3]) == 0) {
        Serial.print("Port: ");
        Serial.println(port);
    } else if (strcmp(args[1], param_args[4]) == 0) {
        Serial.print("Period: ");
        Serial.println(period);
     } else if (strcmp(args[1], param_args[5]) == 0) {
        Serial.print("Buffer: ");
        Serial.println(buffer);
    } else {
        Serial.println("[SHOW] You must specify a valid parameter or none. Type HELP SHOW for more information.");
    }
}

// SET function to alter the specified parameter
int cmd_set() {
    bool error = false;
    if (args[1] == NULL) {
        Serial.println("[SET] You must specify a valid parameter. Type HELP SET for more information");
    } else if (strcmp(args[1], param_args[0]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(ssid, args[2]);
        }
    } else if (strcmp(args[1], param_args[1]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(password, args[2]);
        }
    } else if (strcmp(args[1], param_args[2]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(ipaddress, args[2]);
        }
    } else if (strcmp(args[1], param_args[3]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(port, args[2]);
        }
    } else if (strcmp(args[1], param_args[4]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(period, args[2]);
        }
    } else if (strcmp(args[1], param_args[5]) == 0) {
        if (args[2] == NULL) {
            error = true;
        } else {
            strcpy(buffer, args[2]);
        }
    }
    else {
        Serial.println("[SET] You must specify a valid parameter. Type HELP SET for more information");
    }
    if (error == true) {
        Serial.println("[SET] You must specify a value. Type HELP SET for more information.");
    } else {
        Serial.print("Successfully set ");
        Serial.println(args[2]);
    }
}

// The necessary things that probably already exist, just be sure Serial.begin is in there
void setup() {
    Serial.begin(115200);
    xTaskCreate(
        run_cli,
        "run_cli",
        4000,
        NULL,
        4,
        NULL);
}

void loop() {
    Serial.println(" ");
}
