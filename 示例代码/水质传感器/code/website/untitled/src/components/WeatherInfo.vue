<template>
  <div>
    <h3>天气信息</h3>
    <div v-if="weatherData" class="weather-content">
      <p><strong>城市:</strong> {{ weatherData.city }}</p>
      <p><strong>天气:</strong> {{ weatherData.weather }}</p>
      <p><strong>温度:</strong> {{ weatherData.temperature }} °C</p>
      <p><strong>湿度:</strong> {{ weatherData.humidity }} %</p>
      <p><strong>风向:</strong> {{ weatherData.windDirection }}</p>
      <p><strong>风力:</strong> {{ weatherData.windPower }} 级</p>
    </div>
    <div v-else>
      <p>正在加载天气数据...</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import axios from 'axios';
interface LiveWeatherData {
  province: string;
  city: string;
  adcode: string;
  weather: string;
  temperature: string;
  winddirection: string;
  windpower: string;
  humidity: string;
  reporttime: string;
}

// 用于在模板中显示的数据结构
interface DisplayWeatherData {
  province: string;
  city: string;
  weather: string;
  temperature: string;
  humidity: string;
  windDirection: string;
  windPower: string;
  reportTime: string;
}

const weatherData = ref<DisplayWeatherData | null>(null);
const error = ref<string | null>(null);

const apiKey = 'adaadfc967b05c1e4fd01c0c57ffb739'; // 您提供的高德 Key
const cityAdcode = '440300'; // 深圳市的 Adcode

const fetchWeatherData = async () => {
  error.value = null; // 重置错误信息
  try {
    const response = await axios.get('https://restapi.amap.com/v3/weather/weatherInfo', {
      params: {
        key: apiKey,
        city: cityAdcode,
        extensions: 'base', // 获取实况天气
        output: 'JSON' // 明确指定返回格式为 JSON
      }
    });

    if (response.data && response.data.status === '1' && response.data.lives && response.data.lives.length > 0) {
      const liveInfo: LiveWeatherData = response.data.lives[0];
      weatherData.value = {
        province: liveInfo.province,
        city: liveInfo.city,
        weather: liveInfo.weather,
        temperature: liveInfo.temperature,
        humidity: liveInfo.humidity,
        windDirection: liveInfo.winddirection,
        windPower: liveInfo.windpower,
        reportTime: liveInfo.reporttime
      };
    } else {
      console.error('获取天气数据失败:', response.data);
      error.value = `获取天气数据失败: ${response.data.info || '未知错误'}`;
      weatherData.value = null;
    }
  } catch (err: any) {
    console.error('请求天气API时出错:', err);
    error.value = `请求天气API时出错: ${err.message || '网络错误'}`;
    weatherData.value = null;
  }
};

onMounted(() => {
  fetchWeatherData();
  // 可以设置定时器刷新天气信息，例如每10分钟
  // setInterval(fetchWeatherData, 600000);
});
//
// interface WeatherData {
//   city: string;
//   weather: string;
//   temperature: string;
//   humidity: string;
//   windDirection: string;
//   windPower: string;
// }
//
// const weatherData = ref<WeatherData | null>(null);
//
// // 模拟获取天气信息，实际项目中应替换为真实API调用
// const fetchWeatherData = async () => {
//   // 假设这是从高德天气API获取的数据 (需要替换为真实请求)
//   // 注意：高德Web服务API通常需要后端代理以避免暴露Key和处理跨域
//   // 这里仅为前端模拟
//   await new Promise(resolve => setTimeout(resolve, 1000)); // 模拟网络延迟
//   weatherData.value = {
//     city: '示例城市',
//     weather: '晴',
//     temperature: '28',
//     humidity: '60',
//     windDirection: '东南',
//     windPower: '3',
//   };
// };
//
// onMounted(() => {
//   fetchWeatherData();
//   // 可以设置定时器刷新天气信息
//   // setInterval(fetchWeatherData, 600000); // 每10分钟刷新一次
// });
</script>

<style scoped>
h3 {
  color: #64FFDA;
  margin-bottom: 10px;
  border-bottom: 1px solid #173A5E;
  padding-bottom: 5px;
}
.weather-content p {
  margin: 8px 0;
  font-size: 0.9em;
  line-height: 1.6;
}
.weather-content strong {
  color: #8892B0; /* 标签颜色 */
}
</style>

