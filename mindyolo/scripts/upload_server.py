from flask import Flask, request

app = Flask(__name__)

@app.route("/upload_yichang", methods=["POST"])
def upload():
    data = request.get_data()
    print("received:", len(data))
    with open("/home/HwHiAiUser/rabbits/images/yichang.jpg", "wb") as f:
        f.write(data)
    return "OK"

@app.route("/upload_liaocao", methods=["POST"])
def upload():
    data = request.get_data()
    print("received:", len(data))
    with open("/home/HwHiAiUser/rabbits/images/liaocao.jpg", "wb") as f:
        f.write(data)
    return "OK"

app.run(host="0.0.0.0", port=80)
