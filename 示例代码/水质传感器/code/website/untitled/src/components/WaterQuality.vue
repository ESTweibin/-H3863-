<template>
  <div>
    <h3>水质信息</h3>
    <div class="water-stats">
      <div class="stat-item">温度: {{ waterData.temperature }} °C</div>
      <div class="stat-item">pH: {{ waterData.ph }}</div>
      <div class="stat-item">溶氧量: {{ waterData.dissolvedOxygen }} mg/L</div>
      <div class="stat-item">浑浊度: {{ waterData.turbidity }} NTU</div>
      <div class="stat-item">电导率: {{ waterData.conductivity }} μS/cm</div>
    </div>
    <div ref="chartRef" style="width: 100%; height: 200px;"></div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, reactive } from 'vue';
import * as echarts from 'echarts';
import {
  getWaterQualityReadings,
  getWaterQualityReadingsLatest
} from "../api/water_quality_readings.ts";

const chartRef = ref<HTMLElement | null>(null);
let myChart: echarts.ECharts | null = null;
// 模拟水质数据
const waterData = reactive({
  temperature: 0,
  ph: 0,
  dissolvedOxygen: 0,
  turbidity: 0,
  conductivity: 0,
});

const historyData = reactive<{
  time: string[];
  temperature: (number | null)[];
  ph: (number | null)[];
  dissolvedOxygen: (number | null)[];
}>({
  time: [],
  temperature: [],
  ph: [],
  dissolvedOxygen: [],
});
const fetchAndUpdateLatestWaterData = async () => {
  try {
    const response = await getWaterQualityReadingsLatest(); // 假设这个接口返回最新的单个数据点
    if (response && response.data) {
      const latest = response.data;
      waterData.temperature = latest.temperature_celsius ?? 0;
      waterData.ph = latest.ph_value ?? 0;
      waterData.dissolvedOxygen = latest.dissolved_oxygen_mgl ?? 0;
      waterData.turbidity = latest.turbidity_ntu ?? 0;
      waterData.conductivity = latest.conductivity_us_cm ?? 0;
    }
  } catch (error) {
    console.error("获取最新水质数据失败:", error);
  }
};

const fetchHistoryWaterData = async () => {
  try {
    const response = await getWaterQualityReadings();
    if (response && response.data && Array.isArray(response.data)) {
      // 清空旧数据
      historyData.time = [];
      historyData.temperature = [];
      historyData.ph = [];
      historyData.dissolvedOxygen = [];

      // 对数据按时间戳升序排序
      const sortedData = response.data.sort((a: any, b: any) => new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime());

      sortedData.forEach((reading: any) => {
        historyData.time.push(new Date(reading.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
        historyData.temperature.push(reading.temperature_celsius);
        historyData.ph.push(reading.ph_value);
        historyData.dissolvedOxygen.push(reading.dissolved_oxygen_mgl);
      });

      if (myChart) {
        myChart.setOption({
          xAxis: {
            data: historyData.time,
          },
          series: [
            {
              name: '温度',
              data: historyData.temperature,
            },
            {
              name: 'pH',
              data: historyData.ph,
            },
            {
              name: '溶氧量',
              data: historyData.dissolvedOxygen,
            },
          ],
        });
      }
    }
  } catch (error) {
    console.error("获取历史水质数据失败:", error);
  }
};
onMounted(async () => {
  if (chartRef.value) {
    myChart = echarts.init(chartRef.value);
    const option = {
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['温度', 'pH', '溶氧量'],
        textStyle: {
          color: '#E0E0E0'
        }
      },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        containLabel: true
      },
      xAxis: {
        type: 'category',
        boundaryGap: false,
        data: historyData.time, // 初始为空
        axisLine: {
          lineStyle: {
            color: '#E0E0E0'
          }
        }
      },
      yAxis: {
        type: 'value',
        axisLine: {
          lineStyle: {
            color: '#E0E0E0'
          }
        },
        splitLine: {
          lineStyle: {
            color: '#2A3F54' // 网格线颜色
          }
        }
      },
      series: [
        {
          name: '温度',
          type: 'line',
          smooth: true,
          data: historyData.temperature, // 初始为空
          itemStyle: { color: '#64FFDA' }
        },
        {
          name: 'pH',
          type: 'line',
          smooth: true,
          data: historyData.ph, // 初始为空
          itemStyle: { color: '#FFD700' }
        },
        {
          name: '溶氧量',
          type: 'line',
          smooth: true,
          data: historyData.dissolvedOxygen, // 初始为空
          itemStyle: { color: '#1890FF' }
        }
      ],
      backgroundColor: 'transparent'
    };
    myChart.setOption(option);

    // 获取初始数据
    await fetchAndUpdateLatestWaterData();
    await fetchHistoryWaterData();

    // 移除或修改模拟数据更新的 setInterval
    // 如果需要定时刷新，可以调用 fetchAndUpdateLatestWaterData 和 fetchHistoryWaterData
    setInterval(async () => {
      await fetchAndUpdateLatestWaterData();
      // 如果历史数据也需要定时刷新，取消下一行的注释
      // await fetchHistoryWaterData();

      // 如果图表需要动态添加新点而不是完全重绘历史数据，这里的逻辑需要调整
      // 例如，只获取最新的一个点，然后追加到 historyData 并更新图表
      // 当前的 fetchHistoryWaterData 会重新获取所有历史数据

    }, 5000); // 例如每5秒更新一次最新数据
  }
});
// onMounted(() => {
//   if (chartRef.value) {
//     const myChart = echarts.init(chartRef.value);
//     const option = {
//       tooltip: {
//         trigger: 'axis'
//       },
//       legend: {
//         data: ['温度', 'pH', '溶氧量'],
//         textStyle: {
//             color: '#E0E0E0'
//         }
//       },
//       grid: {
//         left: '3%',
//         right: '4%',
//         bottom: '3%',
//         containLabel: true
//       },
//       xAxis: {
//         type: 'category',
//         boundaryGap: false,
//         data: historyData.time,
//         axisLine: {
//             lineStyle: {
//                 color: '#E0E0E0'
//             }
//         }
//       },
//       yAxis: {
//         type: 'value',
//         axisLine: {
//             lineStyle: {
//                 color: '#E0E0E0'
//             }
//         },
//         splitLine: {
//             lineStyle: {
//                 color: '#2A3F54' // 网格线颜色
//             }
//         }
//       },
//       series: [
//         {
//           name: '温度',
//           type: 'line',
//           smooth: true,
//           data: historyData.temperature,
//           itemStyle: { color: '#64FFDA' }
//         },
//         {
//           name: 'pH',
//           type: 'line',
//           smooth: true,
//           data: historyData.ph,
//           itemStyle: { color: '#FFD700' }
//         },
//         {
//           name: '溶氧量',
//           type: 'line',
//           smooth: true,
//           data: historyData.dissolvedOxygen,
//           itemStyle: { color: '#1890FF' }
//         }
//       ],
//       backgroundColor: 'transparent' // 图表背景透明
//     };
//     myChart.setOption(option);




</script>

<style scoped>
h3 {
  color: #64FFDA;
  margin-bottom: 10px;
  border-bottom: 1px solid #173A5E;
  padding-bottom: 5px;
}
.water-stats {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 8px;
  margin-bottom: 15px;
  font-size: 0.9em;
}
.stat-item {
  background-color: #0A192F;
  padding: 8px;
  border-radius: 4px;
  border: 1px solid #173A5E;
}
</style>

