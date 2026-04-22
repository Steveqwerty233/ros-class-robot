#!/usr/bin/env python3
from openai import OpenAI
import rospy
from std_msgs.msg import String

api_key = "sk-tL8asAb9ybjOx89F7RJk01fO5sPzNPT5O9sr20xlCnqp5p2k"
base_url = "https://api.moonshot.cn/v1"


class LLM(OpenAI):
    def __init__(self):
        super().__init__(api_key=api_key, base_url=base_url)
        self.model = "moonshot-v1-8k"
        self.system_role_content = (
            "你是 Kimi, 由 Moonshot AI 提供的人工智能助手, "
            "我们将会叫你的小名“小月”, 你不会在你的回答中提及你的小名, 你更擅长中文和 "
            "英文的对话. "
            "你会为用户提供安全, 有帮助, 准确的回答. "
            "同时, 你会拒绝一切涉及恐怖主义, 种族歧视, 黄色暴力等问题的回答"
        )

        rospy.init_node('robot_voice_llm_node', anonymous=True)
        self.talk_pub = rospy.Publisher("/talk", String, queue_size=10)
        rospy.Subscriber("/speech/result", String, self.speech_result_callback)

        rospy.sleep(0.5)  # 给 publisher 一点时间建立连接

    def speech_result_callback(self, msg):
        result = msg.data.strip()
        print("speech [{}]".format(result))

        if result:
            try:
                chat_response = self.query(result)
                indented_response = "\n".join(f"\t{line}" for line in chat_response.splitlines())
                print(f"LLM 的返回结果: \n\n'''\n{indented_response}\n'''")

                tts_msg = String()
                tts_msg.data = chat_response
                self.talk_pub.publish(tts_msg)
                print("已发布到 /talk，开始语音播报")

            except Exception as e:
                if "rate_limit_reached" in str(e):
                    print("请求超限")
                else:
                    print("出错啦: {}".format(e))

    def get_system_role_prompt(self):
        return {"role": "system", "content": self.system_role_content}

    def user_prompt(self, user_prompt):
        return {"role": "user", "content": user_prompt}

    def query(self, user_prompt):
        user_message = [self.get_system_role_prompt(), self.user_prompt(user_prompt)]
        completion = self.chat.completions.create(
            model=self.model,
            messages=user_message,
            temperature=0.1,
            stream=False
        )
        return completion.choices[0].message.content.strip()


if __name__ == "__main__":
    try:
        llm = LLM()
        rospy.spin()
    except KeyboardInterrupt:
        print("\nCaught Ctrl + C. Exiting")
