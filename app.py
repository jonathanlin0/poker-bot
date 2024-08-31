from flask import Flask, render_template, request, Response
import subprocess
import os
import json
import random
import string

app = Flask(__name__)

current_file_directory = os.path.dirname(os.path.abspath(__file__))

# Load configuration from the website.json file
def load_config():
    with open(os.path.join(current_file_directory, 'conf/website.json'), 'r') as f:
        return json.load(f)

# Load training configuration, including reward functions, model version, weight saving interval, and exploit test multipliers
def load_training_config():
    with open(os.path.join(current_file_directory, 'conf/training.json'), 'r') as f:
        return json.load(f)

# Generate a random string of the specified length
def get_random_string(length):
    letters = string.ascii_lowercase
    result_str = ''.join(random.choice(letters) for i in range(length))
    return result_str

@app.route("/")
def index():
    config = load_config()
    training_config = load_training_config()
    
    reward_functions = training_config["reward_functions"]
    model_version = training_config["model_version"]
    save_weights_interval = [f"{interval:,}" for interval in training_config["save_weights_intervals"]]  # Format save weights intervals with commas
    exploit_test_multipliers = training_config["exploit_test_multipliers"]  # Get exploit test multipliers

    run_id_length = config.get("run_id_length", 16)  # Default to 16 if not specified
    run_id = get_random_string(run_id_length)
    
    return render_template("index.html", 
                           reward_functions=reward_functions, 
                           model_version=model_version, 
                           run_id=run_id, 
                           save_weights_interval=save_weights_interval,
                           exploit_test_multipliers=exploit_test_multipliers)  # Pass exploit test multipliers to the template

def generate_output():
    # Start the subprocess with unbuffered output
    process = subprocess.Popen(["python3", "-u", "mock_training_script.py"], cwd=current_file_directory, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # Yield each line as it comes
    for line in process.stdout:
        yield line
    process.stdout.close()
    return_code = process.wait()
    yield f'Process exited with code {return_code}\n'

@app.route("/start", methods=["POST"])
def start():
    # Retrieve form data
    run_id = request.form.get('run_id')
    model_version = request.form.get('model_version')
    reward_function = request.form.get('rewardFunction')
    save_weights_interval = request.form.get('saveWeightsInterval')  # Retrieve save weights interval from form
    exploit_test_multiplier = request.form.get('exploitTestMultiplier')  # Retrieve exploit test multiplier from form
    additional_notes = request.form.get('additionalNotes')

    # Print the settings to the console (for backend visibility)
    print(f"Run ID: {run_id}")
    print(f"Model Version: {model_version}")
    print(f"Reward Function: {reward_function}")
    print(f"Save Weights Interval: {save_weights_interval}")
    print(f"Exploit Test Multiplier: {exploit_test_multiplier}")
    print(f"Additional Notes: {additional_notes}")

    # Start streaming output from the temp.py script
    return Response(generate_output(), mimetype="text/plain")

if __name__ == "__main__":
    app.run(debug=True)
