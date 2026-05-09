# Made by ChatGPT, Edited and reviewed by Malek

import os
import time
import subprocess

RESET   = "\033[0m"
BOLD    = "\033[1m"
DIM     = "\033[2m"

RED     = "\033[91m"
GREEN   = "\033[92m"
YELLOW  = "\033[93m"
BLUE    = "\033[94m"
MAGENTA = "\033[95m"
CYAN    = "\033[96m"
WHITE   = "\033[97m"

TITLE = f"""
{WHITE}██╗  ██╗{WHITE}██╗   ██╗{WHITE}██████╗ {WHITE}███╗   ███╗{WHITE} ██████╗ {WHITE}██████╗  
{WHITE}██║  ██║{WHITE}██║   ██║{WHITE}██╔══██╗{WHITE}████╗ ████║{WHITE}██╔═══██╗{WHITE}██╔══██╗ 
{WHITE}███████║{WHITE}██║   ██║{WHITE}██║  ██║{WHITE}██╔████╔██║{WHITE}██║   ██║{WHITE}██║  ██║ 
{WHITE}██╔══██║{WHITE}██║   ██║{WHITE}██║  ██║{WHITE}██║╚██╔╝██║{WHITE}██║   ██║{WHITE}██║  ██║ 
{WHITE}██║  ██║{WHITE}╚██████╔╝{WHITE}██████╔╝{WHITE}██║ ╚═╝ ██║{WHITE}╚██████╔╝{WHITE}██████╔╝ 
{WHITE}╚═╝  ╚═╝ {WHITE}╚═════╝ {WHITE}╚═════╝ {WHITE}╚═╝     ╚═╝{WHITE} ╚═════╝ {WHITE}╚═════╝  

{DIM}               {MAGENTA}G D E x t e n s i o n{RESET}
"""

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def separator():
    print(f"{DIM}{'-' * 55}{RESET}")


def typewriter(text, delay=0.01, color=WHITE):
    print(color, end="")
    for char in text:
        print(char, end="", flush=True)
        time.sleep(delay)
    print(RESET)


def spinner(seconds=1.5):
    chars = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
    end_time = time.time() + seconds

    i = 0
    while time.time() < end_time:
        print(f"\r{CYAN}{chars[i % len(chars)]} Initializing...{RESET}", end="")
        time.sleep(0.08)
        i += 1

    print("\r", end="")


def ask_choice(title, options):
    print(f"\n{BOLD}{CYAN}{title}{RESET}\n")

    for i, option in enumerate(options, start=1):
        print(f"  {YELLOW}[{i}]{RESET} {WHITE}{option}{RESET}")

    while True:
        choice = input(f"\n{GREEN}> Select option:{RESET} ")

        if choice.isdigit():
            choice = int(choice)

            if 1 <= choice <= len(options):
                return options[choice - 1]

        print(f"{RED}Invalid choice, try again.{RESET}")


def main():
    clear()

    print(TITLE)

    spinner()

    typewriter("Loading build environment...", 0.015, CYAN)

    separator()

    platform = ask_choice(
        "Choose Target Platform",
        [
            "linux",
            "windows"
        ]
    )

    separator()

    arch = ask_choice(
        "Choose Architecture",
        [
            "x86_64",
            "x86_32",
            "arm64"
        ]
    )

    separator()

    build_type = ask_choice(
        "Choose Build Type",
        [
            "debug",
            "release"
        ]
    )

    separator()

    use_mingw = "no"

    if platform == "windows":
        use_mingw = ask_choice(
            "Use MinGW?",
            [
                "yes",
                "no"
            ]
        )

    separator()

    command = ["scons"]

    command.append(f"platform={platform}")
    command.append(f"arch={arch}")

    if build_type == "release":
        command.append("release=yes")

    if platform == "windows":
        command.append(f"use_mingw={use_mingw}")

    print(f"\n{BOLD}{MAGENTA}Build Configuration{RESET}\n")

    print(f"{CYAN}Platform:{RESET}      {WHITE}{platform}{RESET}")
    print(f"{CYAN}Architecture:{RESET}  {WHITE}{arch}{RESET}")
    print(f"{CYAN}Build Type:{RESET}    {WHITE}{build_type}{RESET}")

    if platform == "windows":
        print(f"{CYAN}Use MinGW:{RESET}    {WHITE}{use_mingw}{RESET}")

    separator()

    print(f"\n{GREEN}Build Command:{RESET}\n")
    print(f"{YELLOW}{' '.join(command)}{RESET}")

    separator()

    confirm = input(f"\n{GREEN}> Start build? (y/n): {RESET}")

    if confirm.lower() != "y":
        print(f"\n{RED}Build cancelled.{RESET}\n")
        return


    print(f"\n{CYAN}Starting build process...{RESET}\n")

    try:
        result = subprocess.run(command)

        if result.returncode == 0:
            print(f"\n{GREEN}{BOLD}✔ Build completed successfully!{RESET}\n")
        else:
            print(f"\n{RED}{BOLD}✖ Build failed!{RESET}\n")

    except KeyboardInterrupt:
        print(f"\n{RED}Build interrupted by user.{RESET}\n")


if __name__ == "__main__":
    main()
