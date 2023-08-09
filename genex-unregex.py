#!/usr/bin/env python3

import re
import random
import string



# Test input generation

def random_string(r = range(0, 12)):
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(random.choice(r)))

def generate_random_json_inputs(n = 5):
    header = "{'username': '"
    middle = "', 'password': '"
    footer = "'}"
    return [(header + random_string() + middle + random_string() + footer) for _ in range(n)]

def generate_random_http_inputs(n = 5):
    header = "GET /echo?a="
    middle = "&b="
    footer = " HTTP/1.1\r\nHost: reqbin.com\r\nAccept: */*"
    return [(header + random_string() + middle + random_string() + footer) for _ in range(n)]

def generate_random_long_http_inputs(n = 5):
    inputs = []
    for _ in range(n):
        username = random_string(range(4, 12))
        password = random_string(range(6, 18))
        user_agent_version = random.choice(string.digits) + "." + random.choice(string.digits)
        payload = "username=" + username + "&password=" + password
        inputs.append("POST /cgi-bin/process.cgi HTTP/1.1\r\nUser-Agent: Mozilla/" + user_agent_version + "\r\nHost: 10.60.0.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: " + str(len(payload)) + "\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Keep-Alive\r\n\r\n" + payload)
    return inputs


# This is the main algorithm

# Generate a regex that closely matches the inputs
# inputs: a list of raw (unescaped) strings
# returns: a regex string
def analyze(inputs: list):
    inputs = "\n".join(pre_process(inputs))
    regex = build_regex(inputs)
    generalized = generalize(regex)
    processed = post_process(generalized)
    return processed

# Regex-escape and newline-escape inputs
# inputs: a list of raw (unescaped) strings
# returns: a list of escaped strings
def pre_process(inputs : str):
    return [re.escape(i.encode("unicode_escape").decode("utf-8")) for i in inputs]

def post_process(regex: str):
    # return regex.encode("utf_8").decode("unicode_escape")
    return regex

# Build a regex that closely matches the inputs
# This is the main algorithm
# For reasons involving the complexity of un-escaping regex special characters
# inputs: a string of regex-escaped inputs separated by newlines
# returns: a regex string
def build_regex(inputs: str):
    if not max(len(i) for i in inputs.split("\n")):
        return ""
    input_count = inputs.count("\n")

    upper_bound = min(len(i) for i in inputs.split("\n"))
    lower_bound = 0

    # Do a binary search to find the biggest search_len that results in re.search(regex,inputs) returning true
    while True:
        search_len = (upper_bound + lower_bound) // 2            
        regex = r"(.*?)(.{" + str(search_len) + r",})(.*)" + (r"\n(.*?)\2(.*)" * input_count)
        if re.fullmatch(regex, inputs):
            lower_bound = search_len
        else:
            upper_bound = search_len
        if upper_bound - lower_bound <= 1:
            break

    if search_len < 2:
        return "(" + "|".join(inputs.split("\n")) + ")"
    
    regex = r"(.*?)(.{" + str(search_len-1) + r",})(.*)" + (r"\n(.*?)\2(.*)" * input_count)
    shared = re.sub(regex, r"\2", inputs)
    pre_regex = r"\g<1>"
    post_regex = r"\g<3>"
    for i in range(input_count):
        pre_regex += "\\n\\g<" + str((i * 2) + 4) + ">"
        post_regex += "\\n\\g<" + str((i * 2) + 5) + ">"
    pre_diff = re.sub(regex, pre_regex, inputs)
    post_diff = re.sub(regex, post_regex, inputs)
    pre_merged = build_regex(pre_diff)
    post_merged = build_regex(post_diff)
    return pre_merged + shared + post_merged


def generalize(regex: str):
    # Extract the contents of all capture groups from the regex
    # regex: a regex string

    charsets = [
        "a-z",
        "A-Z",
        "0-9",
    ]
    
    # Extract everything inside capture groups (this is a complicatd regex because it has to deal with escaped parentheses, and escaped backslashes)
    groups = re.findall(r"\(((?:[^\)\\]|\\\)|\\\\)*)\)", regex)
    for group in groups:
        options = re.findall(r"((?:[^\|\\]|\\\||\\\\)*)\|", group + "|")
        descriptor = ""
        for charset in charsets:
            if any(re.search("[" + charset + "]", option) for option in options):
                descriptor += charset
        if descriptor:
            descriptor = "[" + descriptor + "]"
        else:
            descriptor = "(" + group + ")"
        descriptor += "{" + str(min(len(option) for option in options)) + "," + str(max(len(option) for option in options)) + "}"
        regex = re.sub(r"\(" + re.escape(group) + r"\)", descriptor, regex)

    # Clean up any single-length groups or optional groups
    regex = re.sub(r"\{0,1\}", r"?", regex)
    regex = re.sub(r"\{1,1\}", r"", regex)
    regex = re.sub(r"\{(\d+)\,\1\}", r"{\1}", regex)
    
    return regex



def test():
    # inputs = generate_random_http_inputs()
    inputs = generate_random_json_inputs(1000)
    # inputs = generate_random_long_http_inputs(1000)

    print("inputs:")
    for i in inputs:
        print(i)
    print()

    print("regex:")
    result = analyze(inputs)

    print(result)

def write_tests():
    inputs = generate_random_long_http_inputs(1000)
    # Write each input to its own numbered file in tests/long_http
    for i in range(len(inputs)):
        with open("tests/long_http/" + str(i), "w") as f:
            f.write(inputs[i])


if __name__ == "__main__":
    test()
    # write_tests()