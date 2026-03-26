<template>
  <div>
    <h3>治理前后福寿螺数量对比</h3>
    <div ref="chartRef" style="width: 100%; height: 250px;"></div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, reactive } from 'vue';
import * as echarts from 'echarts';
import {getTreatmentRecords} from "../api/treatment_records.ts";
const chartRef = ref<HTMLElement | null>(null);
let myChart: echarts.ECharts | null = null;

// 定义图表数据
const chartData = reactive<{
  categories: string[];
  beforeTreatment: (number | null)[];
  afterTreatment: (number | null)[];
}>({
  categories: [],
  beforeTreatment: [],
  afterTreatment: [],
});

// 获取并处理治理记录数据
const fetchAndUpdateChartData = async () => {
  try {
    const response = await getTreatmentRecords();
    if (response && response.data && Array.isArray(response.data)) {
      // 按 treatment_record_id 降序排序以获取最新的记录，如果需要按日期，请使用 treatment_date 或 created_at
      const sortedRecords = response.data.sort((a: any, b: any) => b.treatment_record_id - a.treatment_record_id);

      // 取最新的5条记录
      const latestRecords = sortedRecords.slice(0, 5);

      // 清空旧数据
      chartData.categories = [];
      chartData.beforeTreatment = [];
      chartData.afterTreatment = [];

      latestRecords.forEach((record: any) => {
        // 假设 efficacy_notes 存储的是区域名称，如果不是，您需要调整这里的逻辑
        chartData.categories.push(record.efficacy_notes || `记录ID ${record.treatment_record_id}`);
        chartData.beforeTreatment.push(record.count_before_treatment);
        chartData.afterTreatment.push(record.count_after_treatment);
      });

      // 反转数组使图表从左到右显示时间上更早的记录（可选，取决于您的偏好）
      // chartData.categories.reverse();
      // chartData.beforeTreatment.reverse();
      // chartData.afterTreatment.reverse();

      if (myChart) {
        myChart.setOption({
          xAxis: {
            data: chartData.categories,
          },
          series: [
            {
              name: '治理前',
              data: chartData.beforeTreatment,
            },
            {
              name: '治理后',
              data: chartData.afterTreatment,
            },
          ],
        });
      }
    }
  } catch (error) {
    console.error("获取治理记录失败:", error);
    // 可以设置一些默认或错误状态的图表数据
    chartData.categories = ['获取失败'];
    chartData.beforeTreatment = [0];
    chartData.afterTreatment = [0];
    if (myChart) {
      myChart.setOption({
        xAxis: { data: chartData.categories },
        series: [
          { name: '治理前', data: chartData.beforeTreatment },
          { name: '治理后', data: chartData.afterTreatment },
        ],
      });
    }
  }
};

onMounted(async () => {
  if (chartRef.value) {
    myChart = echarts.init(chartRef.value);
    const option = {
      tooltip: {
        trigger: 'axis',
        axisPointer: {
          type: 'shadow'
        }
      },
      legend: {
        data: ['治理前', '治理后'],
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
        data: chartData.categories, // 初始为空
        axisLine: {
          lineStyle: {
            color: '#E0E0E0'
          }
        },
        axisTick: {
          alignWithLabel: true
        },
        axisLabel: {
          interval: 0, //确保所有标签都显示，如果标签过多可能需要旋转或其他处理
          // rotate: 30 // 如果标签名太长，可以旋转
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
            color: '#2A3F54'
          }
        }
      },
      series: [
        {
          name: '治理前',
          type: 'bar',
          barWidth: '30%',
          data: chartData.beforeTreatment, // 初始为空
          itemStyle: {
            color: '#FF6B6B' // 红色系
          }
        },
        {
          name: '治理后',
          type: 'bar',
          barWidth: '30%',
          data: chartData.afterTreatment, // 初始为空
          itemStyle: {
            color: '#64FFDA' // 青色系
          }
        }
      ],
      backgroundColor: 'transparent'
    };
    myChart.setOption(option);

    // 获取并更新图表数据
    await fetchAndUpdateChartData();
  }
});
// onMounted(() => {
//   if (chartRef.value) {
//     const myChart = echarts.init(chartRef.value);
//     const option = {
//       tooltip: {
//         trigger: 'axis',
//         axisPointer: {
//           type: 'shadow'
//         }
//       },
//       legend: {
//         data: ['治理前', '治理后'],
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
//         data: ['区域A', '区域B', '区域C', '区域D', '区域E'],
//         axisLine: {
//             lineStyle: {
//                 color: '#E0E0E0'
//             }
//         },
//         axisTick: {
//           alignWithLabel: true
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
//                 color: '#2A3F54'
//             }
//         }
//       },
//       series: [
//         {
//           name: '治理前',
//           type: 'bar',
//           barWidth: '30%',
//           data: [120, 200, 150, 80, 70],
//           itemStyle: {
//             color: '#FF6B6B' // 红色系
//           }
//         },
//         {
//           name: '治理后',
//           type: 'bar',
//           barWidth: '30%',
//           data: [30, 50, 40, 20, 15],
//           itemStyle: {
//             color: '#64FFDA' // 青色系
//           }
//         }
//       ],
//       backgroundColor: 'transparent'
//     };
//     myChart.setOption(option);
//   }
// });
</script>

<style scoped>
h3 {
  color: #64FFDA;
  margin-bottom: 15px;
  border-bottom: 1px solid #173A5E;
  padding-bottom: 5px;
}
</style>

