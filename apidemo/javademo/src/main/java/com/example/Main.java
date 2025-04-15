// 所需的okhttp库
package com.example;
import java.io.IOException;

import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

public class Main {
    
    public static void main(String[] args) {
        try {
            // 调试输出信息
            String token = fetchAuthToken();
            System.out.println("token: " + token);
            String response = getshuxing(token);
            System.out.println("response: " + response);
            int status = postmessage(token);
            System.out.println("status: " + status);


        } catch (IOException e) {
            System.err.println("请求发生错误: " + e.getMessage());
            e.printStackTrace();
        }
    }

    // post发起鉴权请求并返回响应头中的鉴权 Token 数据
    public static String fetchAuthToken() throws IOException {
        OkHttpClient client = new OkHttpClient();
        
        // 请求鉴权的 URL
        String url = "https://iam.cn-north-4.myhuaweicloud.com/v3/auth/tokens";
        
        // 构造 JSON 格式的请求数据
        String jsonPayload = "{\n" +
        "  \"auth\": {\n" +
        "    \"identity\": {\n" +
        "      \"methods\": [\"password\"],\n" +
        "      \"password\": {\n" +
        "        \"user\": {\n" +
        "          \"name\": \"qing_lan\",\n" +
        "          \"password\": \"cy383245\",\n" +
        "          \"domain\": {\n" +
        "            \"name\": \"qing_lan\"\n" +
        "          }\n" +
        "        }\n" +
        "      }\n" +
        "    },\n" +
        "    \"scope\": {\n" +
        "      \"project\": {\n" +
        "        \"name\": \"cn-north-4\"\n" +
        "      }\n" +
        "    }\n" +
        "  }\n" +
        "}";
    
        MediaType JSON_TYPE = MediaType.get("application/json; charset=utf-8");
        RequestBody body = RequestBody.create(jsonPayload, JSON_TYPE);
        
        // 构造 POST 请求
        Request request = new Request.Builder()
                .url(url)
                .post(body)
                .addHeader("Content-Type", "application/json") 
                .build();

        Response response = null;
        try {
            response = client.newCall(request).execute();
            
            if (!response.isSuccessful()) {
                throw new IOException("请求失败: " + response.code());
            }
            
            // 从响应头中提取鉴权 Token 数据
            String token = response.header("X-Subject-Token");
            return token;
        } finally {
            if (response != null) {
                response.close();
            }
        }
    }


    // get获取设备属性返回响应体中的json数据
    public static String getshuxing(String token) throws IOException {
        OkHttpClient client = new OkHttpClient();
        
        // 请求 URL
        String url = "https://ef861ca468.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/b3318d2e70ab4740b267ed8955fd7275/devices/67b683d83f28ab3d0384f27e_environment/properties?service_id=get_tha";
        
        
        // 构造 get 请求
        Request request = new Request.Builder()
                .url(url)
                .get()
                .addHeader("Content-Type", "application/json")
                .addHeader("X-Auth-Token", token)  // 这里传递 fetchAuthToken() 方法获取的 token
                .build();

        Response response = null;
        try {
            response = client.newCall(request).execute();
            
            if (!response.isSuccessful()) {
                throw new IOException("请求失败: " + response.code());
            }
            
            // 提取返回数据
            String message = response.body().string();
            return message;
        } finally {
            if (response != null) {
                response.close();
            }
        }
    }

    // post消息返回状态码
    public static int postmessage(String token) throws IOException {
        OkHttpClient client = new OkHttpClient();
        
        // post URL
        String url = "https://ef861ca468.st1.iotda-app.cn-north-4.myhuaweicloud.com:443/v5/iot/b3318d2e70ab4740b267ed8955fd7275/devices/67b683d83f28ab3d0384f27e_environment/messages";
        

        String jsonPayload = "{\n"
        + "  \"message\": {\n"
        + "    \"pwm\": 50,\n"
        + "    \"num\": 1\n"
        + "  }\n"
        + "}";
    
        MediaType JSON_TYPE = MediaType.get("application/json; charset=utf-8");
        RequestBody body = RequestBody.create(jsonPayload, JSON_TYPE);
        
        // 构造 POST 请求
        Request request = new Request.Builder()
                .url(url)
                .post(body)
                .addHeader("Content-Type", "application/json") 
                .addHeader("X-Auth-Token", token)  
                .build();

        Response response = null;
        try {
            response = client.newCall(request).execute();
            
            if (!response.isSuccessful()) {
                throw new IOException("请求失败: " + response.code());
            }
            int status = response.code();
            return status;
        } finally {
            if (response != null) {
                response.close();
            }
        }
    }
}


