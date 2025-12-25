import json
import os
import logging
from typing import Optional
from pydub import AudioSegment
from urllib.request import urlopen, Request
from urllib.error import URLError
from urllib.parse import urlencode, quote_plus
from aip import AipSpeech
from fastapi import FastAPI, UploadFile, File, Form, HTTPException
from fastapi.responses import JSONResponse, FileResponse, Response
from fastapi.staticfiles import StaticFiles
import uvicorn
import base64
import io
import wave
import requests
import socket
# 导入配置模块
try:
    import config
except ImportError:
    # 如果 config.py 不存在，使用默认配置
    import types
    config = types.SimpleNamespace()
    config.LOG_LEVEL = 'INFO'
    config.BAIDU_APP_ID = '118170124'
    config.BAIDU_API_KEY = 'OMC143fzVHzZIItAe4Bp1JW0'
    config.BAIDU_SECRET_KEY = 'Fnp1i9hHoFckvWz66R9oilFF7LLnEkYQ'
    config.Deepseek_API_KEY = 'XSpiaGrzd90ifAwk_8ubeBv87rXLZUOyx_n4p5MrnLhOAXrKqeqOZT4yKZdRuyx8r1TgR-xNQfRaI6rWEbE9-g'
    config.Deepseek_url = 'https://maas-cn-southwest-2.modelarts-maas.com/v1/infers/8a062fd4-7367-4ab4-a936-5eeb8fb821c4/v1/chat/completions'
    config.Deepseek_MODEL = 'DeepSeek-R1'
    config.AI_MODEL_MODE = 'auto'
    config.MINDSPORE_DEVICE = 'CPU'
    config.MINDSPORE_MODEL_PATH = './models/dialog_model.mindir'
    config.SYSTEM_PROMPT = '你是一个养兔专家，专注于解答与养兔相关的问题。回答要求简洁、口语化、有逻辑、具体，长度不超过30字'
    # ModelArts 配置（华为云）
    config.MODELARTS_SERVICE_URL = os.getenv('MODELARTS_SERVICE_URL', '')
    config.MODELARTS_TOKEN = os.getenv('MODELARTS_TOKEN', '')
    config.PER = 4
    config.SPD = 5
    config.PIT = 5
    config.VOL = 5
    config.AUE = 4
    config.FORMAT = 'pcm'
    config.CUID = '123456PYTHON'
    config.TTS_URL = 'http://tsn.baidu.com/text2audio'
    config.TOKEN_URL = 'http://aip.baidubce.com/oauth/2.0/token'
    config.SCOPE = 'audio_tts_post'
    config.SERVER_HOST = '0.0.0.0'
    config.SERVER_PORT = 8005

# 尝试导入 MindSpore 相关模块
try:
    from mindspore_model import get_dialog_model, SimpleDialogModel, MINDSPORE_AVAILABLE as MINDSPORE_RUNTIME_AVAILABLE
    MINDSPORE_AVAILABLE = MINDSPORE_RUNTIME_AVAILABLE
except ImportError as e:
    import logging
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    logging.warning(f"MindSpore 模块导入失败: {e}，将使用 DeepSeek API")
    MINDSPORE_AVAILABLE = False
    get_dialog_model = None
    SimpleDialogModel = None

# 尝试导入 ModelArts 推理模块（华为云）
try:
    from integrations.modelarts_inference import get_modelarts_model
    MODELARTS_AVAILABLE = True
except ImportError as e:
    logging.warning(f"ModelArts 模块导入失败: {e}")
    MODELARTS_AVAILABLE = False
    get_modelarts_model = None

app = FastAPI(title="Rabbit AI 对话系统")

# 创建静态文件目录（如果不存在）
static_dir = os.path.join(os.path.dirname(__file__), "static")
os.makedirs(static_dir, exist_ok=True)

# 挂载静态文件目录
try:
    app.mount("/static", StaticFiles(directory=static_dir), name="static")
except Exception as e:
    logging.warning(f"静态文件目录挂载失败: {e}")

logging.basicConfig(level=getattr(logging, config.LOG_LEVEL, 'INFO'), format='%(asctime)s - %(levelname)s - %(message)s')

# -------------------- 基础路由（便于浏览器/健康检查） --------------------
@app.get("/")
async def root():
    return {
        "ok": True,
        "message": "rabbit-ai service is running",
        "docs": "/docs",
        "openapi": "/openapi.json",
        "api": {"start": {"method": "POST", "path": "/start"}},
    }


@app.get("/healthz")
async def healthz():
    return {"ok": True}


@app.get("/favicon.ico")
async def favicon():
    """返回空响应，避免浏览器请求 favicon 时出现 404"""
    return Response(status_code=204)


@app.get("/chat-page")
async def chat_page():
    """返回文字对话页面"""
    html_path = os.path.join(static_dir, "chat.html")
    if os.path.exists(html_path):
        return FileResponse(html_path)
    else:
        # 如果文件不存在，返回一个简单的内联 HTML
        return JSONResponse(
            content={"error": "聊天页面未找到，请确保 static/chat.html 文件存在"},
            status_code=404
        )

# -------------------- 启动信息辅助 --------------------
def _get_lan_ip() -> Optional[str]:
    """
    尝试获取本机局域网 IP（用于提示访问地址）。
    失败时返回 None，不影响服务启动。
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # 不会真正发包，但能触发系统选择出站网卡，从而得到本机 IP
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return None

# 百度语音识别和合成配置（从 config 读取）
BAIDU_APP_ID = config.BAIDU_APP_ID
BAIDU_API_KEY = config.BAIDU_API_KEY
BAIDU_SECRET_KEY = config.BAIDU_SECRET_KEY
baidu_client = AipSpeech(BAIDU_APP_ID, BAIDU_API_KEY, BAIDU_SECRET_KEY)

# DeepSeek API 配置（从 config 读取，作为备用方案）
Deepseek_API_KEY = config.Deepseek_API_KEY
Deepseek_url = config.Deepseek_url
Deepseek_MODEL = config.Deepseek_MODEL

# 语音合成配置（从 config 读取）
PER = config.PER
SPD = config.SPD
PIT = config.PIT
VOL = config.VOL
AUE = config.AUE
FORMATS = {3: "mp3", 4: "pcm", 5: "pcm", 6: "wav"}
FORMAT = config.FORMAT
CUID = config.CUID
TTS_URL = config.TTS_URL
TOKEN_URL = config.TOKEN_URL
SCOPE = config.SCOPE

# 全局对话模型实例（延迟加载）
dialog_model = None

# 用于控制问答程序的全局变量（如果需要启动单独线程循环交互，可保留）
# is_running = False
# run_lock = threading.Lock()

def fetch_token():
    """获取百度语音合成的访问令牌"""
    params = {'grant_type': 'client_credentials',
              'client_id': BAIDU_API_KEY,
              'client_secret': BAIDU_SECRET_KEY}
    post_data = urlencode(params).encode('utf-8')
    req = Request(TOKEN_URL, post_data)
    try:
        f = urlopen(req, timeout=5)
        result_str = f.read().decode()
    except URLError as err:
        raise Exception(f"Token请求失败: {err}")
    result = json.loads(result_str)
    if ('access_token' in result.keys() and 'scope' in result.keys()):
        if SCOPE not in result['scope'].split(' '):
            raise Exception('scope 不正确')
        logging.info(f"获取token成功: {result['access_token']}, 过期时间: {result['expires_in']}秒")
        return result['access_token']
    else:
        raise Exception('API_KEY 或 SECRET_KEY 配置错误，token信息缺失')

def recognize_speech_file(audio_bytes: bytes) -> str:
    try:
        audio_segment = AudioSegment.from_file(io.BytesIO(audio_bytes))
        audio_segment = audio_segment.set_frame_rate(16000).set_channels(1).set_sample_width(2)
        audio_segment = audio_segment.normalize().low_pass_filter(3000)
        audio_pcm = audio_segment.raw_data

        result = baidu_client.asr(audio_pcm, 'pcm', 16000, {'dev_pid': 1537})
        logging.info(f"百度语音识别返回结果: {result}")
        if 'result' in result and result['result']:
            text = result['result'][0]
            logging.info(f"识别结果: {text}")
            return text
        else:
            raise Exception(f"语音识别失败，返回结果: {result}")
    except Exception as e:
        raise Exception(f"语音识别异常: {e}")


def synthesize_speech_bytes(text: str) -> bytes:
    """
    调用百度语音合成 API，将文本转换为语音，并返回语音数据（二进制）。
    若出错则抛出异常。
    """
    token = fetch_token()
    # 注意：对文本进行 url 编码
    tex = quote_plus(text)
    params = {
        'tok': token,
        'tex': tex,
        'per': PER,
        'spd': SPD,
        'pit': PIT,
        'vol': VOL,
        'aue': AUE,
        'cuid': CUID,
        'lan': 'zh',
        'ctp': 1
    }
    data = urlencode(params).encode('utf-8')
    req = Request(TTS_URL, data)
    try:
        f = urlopen(req)
        result_bytes = f.read()
        logging.info("result_bytes 类型: %s", type(result_bytes))
        logging.info("result_bytes 长度: %d", len(result_bytes))
        headers = {name.lower(): value for name, value in f.headers.items()}
        # 检查返回数据是否为音频
        if 'content-type' not in headers or headers['content-type'].find('audio/') < 0:
            error_msg = result_bytes.decode('utf-8')
            logging.error(f"TTS API 错误: {error_msg}")
            raise Exception("语音合成失败")
        return result_bytes
    except URLError as err:
        raise Exception(f"TTS 请求失败: {err}")


def get_answer_from_Deepseek(question):
    """使用 DeepSeek API 获取回答（备用方案）"""
    headers = {
        'Content-Type': 'application/json',
        'Authorization': f'Bearer {Deepseek_API_KEY}'
    }

    data = {
        "model": Deepseek_MODEL,
        "messages": [
            {"role": "system", "content": config.SYSTEM_PROMPT},
            {"role": "user", "content": question}
        ]
    }

    response = requests.post(Deepseek_url, headers=headers, data=json.dumps(data), verify=False)
    response_data = response.json()
    answer_content = response_data['choices'][0]['message']['content']
    logging.info(f"DeepSeek API 返回: {answer_content}")
    return answer_content


def get_answer(question: str) -> str:
    """
    获取 AI 回答（自动选择最佳可用方案）
    优先级：ModelArts > MindSpore 模型 > DeepSeek API > 简单模型
    """
    global dialog_model
    
    # 方案0: 使用 ModelArts 推理服务（华为云，优先级最高）
    if MODELARTS_AVAILABLE and get_modelarts_model:
        if dialog_model is None:
            try:
                dialog_model = get_modelarts_model()
                if dialog_model:
                    logging.info("已连接 ModelArts 推理服务")
            except Exception as e:
                logging.error(f"ModelArts 初始化失败: {e}")
                dialog_model = None
        
        if dialog_model is not None and config.AI_MODEL_MODE in ['modelarts', 'auto']:
            try:
                answer = dialog_model.predict(question)
                logging.info(f"使用 ModelArts 推理服务生成回答")
                return answer
            except Exception as e:
                logging.error(f"ModelArts 推理失败: {e}，尝试备用方案")
                dialog_model = None  # 重置，下次尝试其他方案
    
    # 明确指定 simple：直接使用简单模型（不依赖模型文件）
    if config.AI_MODEL_MODE == 'simple' and MINDSPORE_AVAILABLE and SimpleDialogModel:
        logging.info("使用 simple 模式：简单模型回答")
        return SimpleDialogModel().predict(question)

    # 初始化 MindSpore/简单模型（如果还未初始化）
    if dialog_model is None and MINDSPORE_AVAILABLE and get_dialog_model:
        try:
            dialog_model = get_dialog_model()
            if dialog_model:
                # get_dialog_model 在没有本地模型时可能会返回 SimpleDialogModel
                if SimpleDialogModel and isinstance(dialog_model, SimpleDialogModel):
                    logging.info("已加载简单模型（无本地 MindIR 模型）")
                else:
                    logging.info("已加载 MindSpore 模型")
        except Exception as e:
            logging.error(f"模型初始化失败: {e}")
            dialog_model = None
    
    # 方案1: 使用 MindSpore 模型
    if dialog_model is not None and config.AI_MODEL_MODE in ['mindspore', 'auto']:
        try:
            answer = dialog_model.predict(question)
            if SimpleDialogModel and isinstance(dialog_model, SimpleDialogModel):
                logging.info("使用简单模型生成回答")
            else:
                logging.info("使用 MindSpore 模型生成回答")
            return answer
        except Exception as e:
            logging.error(f"MindSpore 模型推理失败: {e}，尝试备用方案")
    
    # 方案2: 使用 DeepSeek API
    if config.AI_MODEL_MODE in ['deepseek', 'auto']:
        try:
            answer = get_answer_from_Deepseek(question)
            logging.info(f"使用 DeepSeek API 生成回答")
            return answer
        except Exception as e:
            logging.error(f"DeepSeek API 调用失败: {e}")
            if config.AI_MODEL_MODE == 'deepseek':
                raise  # 如果明确指定使用 DeepSeek，则抛出异常
    
    # 方案3: 使用简单模型（最后备用）
    if MINDSPORE_AVAILABLE and SimpleDialogModel:
        try:
            simple_model = SimpleDialogModel()
            answer = simple_model.predict(question)
            logging.info(f"使用简单模型生成回答")
            return answer
        except Exception as e:
            logging.error(f"简单模型失败: {e}")
    
    # 如果所有方案都失败，返回默认回答
    logging.warning("所有 AI 模型都不可用，返回默认回答")
    return f"抱歉，AI 服务暂时不可用。您的问题是：{question}"

def raw_pcm_to_wav(pcm_bytes: bytes, sample_rate: int, bit_depth: int, channels: int) -> bytes:
    if bit_depth not in [8, 16, 24, 32]:
        raise ValueError(f"Invalid bit depth: {bit_depth}")
    if sample_rate not in [8000, 16000, 44100, 48000]:
        raise ValueError(f"Invalid sample rate: {sample_rate}")
    with io.BytesIO() as wav_buffer:
        with wave.open(wav_buffer, 'wb') as wav_file:
            wav_file.setnchannels(channels)
            wav_file.setsampwidth(bit_depth // 8)
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(pcm_bytes)
        return wav_buffer.getvalue()

def convert_to_wav(audio_bytes: bytes) -> bytes:
    """仅处理非WAV格式的转换"""
    try:
        # 直接返回已为WAV格式的数据
        AudioSegment.from_file(io.BytesIO(audio_bytes), format="wav")
        return audio_bytes
    except Exception:
        # 若需要处理其他格式，在此添加逻辑
        raise ValueError("非WAV格式需提供转换参数")


@app.post("/chat")
async def chat_text(question: str = Form(...)):
    """
    纯文字对话接口：接收文本问题，返回文字回答（不调用语音合成）
    用于简单的文字对话场景
    """
    if not question or not question.strip():
        raise HTTPException(status_code=400, detail="问题不能为空")
    
    logging.info(f"收到文字提问: {question}")
    
    # 调用 AI 模型生成回答（自动选择最佳可用方案）
    try:
        answer_text = get_answer(question.strip())
    except Exception as e:
        logging.error(f"模型生成回答异常: {e}")
        raise HTTPException(status_code=500, detail=f"模型生成回答异常: {e}")
    
    return JSONResponse(content={
        "code": 200,
        "question": question,
        "answer": answer_text,
        "success": True
    })


@app.post("/start")
async def upload_data(
    textData: str = Form(None),
    audioFile: UploadFile = File(None)
):
    """
    POST接口：接收文本或音频数据
    - 若 textData 不为空，则直接作为提问文本；
    - 否则若上传 audioFile，则调用语音识别转换为文本；
    调用 Llama 模型生成回答文本，并调用百度语音合成生成回答音频，
    返回 JSON 格式数据： { code:200, text1: 回答文本, retWav: base64 编码的语音数据, text2: 附加信息 }
    """
    if not textData and not audioFile:
        raise HTTPException(status_code=400, detail="必须上传文本或音频文件")
    if textData:
        question = textData
        logging.info("收到文本提问")
    else:
        try:
            audio_content = await audioFile.read()
            try:
                question = recognize_speech_file(audio_content)
            except Exception as e:
                logging.error(f"音频处理失败: {e}")
                raise HTTPException(500, detail=f"音频转换错误: {str(e)}")
            logging.info("收到语音提问：" + question)
        except Exception as e:
            logging.error(f"语音识别失败: {e}")
            raise HTTPException(status_code=500, detail=f"语音识别失败: {e}")

    if not question:
        raise HTTPException(status_code=400, detail="无法从上传数据中获取提问文本")

    # 调用 AI 模型生成回答（自动选择最佳可用方案）
    try:
        answer_text = get_answer(question)
    except Exception as e:
        logging.error(f"模型生成回答异常: {e}")
        raise HTTPException(status_code=500, detail=f"模型生成回答异常: {e}")

    # 调用百度语音合成将回答文本转换为语音（可选，失败时仍返回文字）
    audio_base64 = None
    duration_seconds = None
    try:
        audio_bytes = synthesize_speech_bytes(answer_text)
        logging.info(f"语音已合成")
        # 将二进制音频数据转换为 base64 编码字符串
        if FORMAT == 'pcm':
            # 百度 PCM 默认参数：16kHz 采样率，16bit，单声道
            audio = AudioSegment(
                data=audio_bytes,
                sample_width=2,
                frame_rate=16000,
                channels=1
            )
        else:
            audio = AudioSegment.from_file(io.BytesIO(audio_bytes), format=FORMAT)
        duration_seconds = len(audio) / 1000  # 转换为秒
        audio_base64 = base64.b64encode(audio_bytes).decode('utf-8')
        logging.info(f"语音长度为:{duration_seconds}秒")
    except Exception as e:
        logging.warning(f"语音合成失败: {e}，将仅返回文字回复")
        # 语音合成失败不影响文字回复，继续返回文字结果

    # 返回结果（优先返回语音，如果语音合成失败则只返回文字）
    response_data = {
        "code": 200,
        "text1": answer_text,
        "text2": f"提问内容: {question}",
        "has_audio": audio_base64 is not None
    }
    
    if audio_base64:
        response_data["retWav"] = audio_base64
        response_data["duration_seconds"] = duration_seconds
    else:
        response_data["audio_error"] = "语音合成服务暂时不可用，仅提供文字回复"
    
    return JSONResponse(content=response_data)

@app.on_event("startup")
async def startup_event():
    """服务启动时的初始化"""
    logging.info("=" * 50)
    logging.info("AI 对话系统启动中...")
    logging.info(f"AI 模型模式: {config.AI_MODEL_MODE}")
    logging.info(f"MindSpore 设备: {config.MINDSPORE_DEVICE}")
    logging.info(f"MindSpore 可用: {MINDSPORE_AVAILABLE}")
    logging.info(f"ModelArts 可用: {MODELARTS_AVAILABLE}")
    
    # 预加载模型（优先级：ModelArts > MindSpore）
    global dialog_model
    
    # 优先尝试 ModelArts
    if MODELARTS_AVAILABLE and get_modelarts_model:
        try:
            dialog_model = get_modelarts_model()
            if dialog_model:
                logging.info("✓ ModelArts 推理服务连接成功")
        except Exception as e:
            logging.warning(f"⚠ ModelArts 连接失败: {e}")
    
    # 如果 ModelArts 不可用，尝试 MindSpore
    if dialog_model is None and config.AI_MODEL_MODE in ['mindspore', 'auto'] and MINDSPORE_AVAILABLE and get_dialog_model:
        try:
            dialog_model = get_dialog_model()
            if dialog_model:
                logging.info("✓ MindSpore 模型加载成功")
            else:
                logging.info("⚠ MindSpore 模型不可用，将使用备用方案")
        except Exception as e:
            logging.warning(f"⚠ 模型预加载失败: {e}，将在首次请求时重试")

    host = str(getattr(config, "SERVER_HOST", "0.0.0.0"))
    port = int(getattr(config, "SERVER_PORT", 8005))
    if host in {"0.0.0.0", "::"}:
        logging.info(f"服务已监听: http://{host}:{port}（绑定所有网卡：这是监听地址，浏览器不要用它访问）")
        logging.info(f"本机访问: http://127.0.0.1:{port}")
        lan_ip = _get_lan_ip()
        if lan_ip and lan_ip != "127.0.0.1":
            logging.info(f"局域网访问: http://{lan_ip}:{port}")
    else:
        logging.info(f"访问地址: http://{host}:{port}")
    logging.info("=" * 50)


if __name__ == "__main__":
    uvicorn.run(app, host=config.SERVER_HOST, port=config.SERVER_PORT)
