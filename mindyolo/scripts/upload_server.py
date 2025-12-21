from flask import Flask, request

app = Flask(__name__)

@app.route("/upload", methods=["POST"])
def upload():
    data = request.get_data()
    print("received:", len(data))
    with open("/home/HwHiAiUser/rabbits/images/img.jpg", "wb") as f:
        f.write(data)
    return "OK"

app.run(host="0.0.0.0", port=80)
