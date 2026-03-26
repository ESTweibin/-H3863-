<template>
  <div>
    <h3><i class="el-icon-warning-outline"></i> 异常警报</h3>
    <el-table :data="alerts" style="width: 100%" height="250" class="alert-table">
      <el-table-column prop="time" label="时间" width="100"></el-table-column>
      <el-table-column prop="location" label="经纬度" width="150"></el-table-column>
      <el-table-column prop="count" label="福寿螺数量"></el-table-column>
    </el-table>
  </div>
</template>

<script setup lang="ts">
import {onMounted, onUnmounted, ref} from 'vue';
import { ElTable, ElTableColumn } from 'element-plus';
import {getAlerts} from "../api/alerts.ts";
import {getSnailObservation} from "../api/snail_observations.ts";

interface Alert {
  id: number;
  time: string;
  location: string;
  count: number;
}

const alerts = ref<Alert[]>([
]);

const fetchAlerts = async () => {
  try {
    const response = await getAlerts();
    if (response && response.data && Array.isArray(response.data)) {
      const alertPromises = response.data.map(async (item: any) => {
        let count = null;
        if (item.snail_observation_id) {
          try {
            // 等待 getSnailObservation 解析
            const snailObservationResponse = await getSnailObservation(item.snail_observation_id);
            if (snailObservationResponse && snailObservationResponse.data) {
              count = snailObservationResponse.data.apple_snail_count;
            }
          } catch (e) {
            console.error(`获取福寿螺观测数据失败 (ID: ${item.snail_observation_id}):`, e);
            // 如果获取失败，count 将保持为 null
          }
        }
        return {
          id: item.alert_id,
          time: new Date(item.updated_at).toLocaleString(),
          location: `${parseFloat(item.latitude).toFixed(4)}, ${parseFloat(item.longitude).toFixed(4)}`,
          count: count,
        };
      });
      // 等待所有警报数据（包括福寿螺数量）处理完毕
      alerts.value = (await Promise.all(alertPromises)).sort((a, b) => b.id - a.id);
    }
  } catch (error) {
    console.error("获取警报失败:", error);
    // 可以添加一些用户提示，例如使用 Element Plus 的 ElMessage
  }
};
let intervalId: number | undefined = undefined;
onMounted(() => {
  fetchAlerts(); // 组件加载时获取一次
  intervalId = window.setInterval(() => {
    fetchAlerts();
  }, 10000); // 每10秒获取一次
});
onUnmounted(() => {
  if (intervalId) {
    clearInterval(intervalId); // 组件卸载时清除定时器
  }
});


</script>

<style scoped>
h3 {
  color: #FF6B6B; /* 警报标题用醒目的红色 */
  margin-bottom: 10px;
  border-bottom: 1px solid #173A5E;
  padding-bottom: 5px;
  display: flex;
  align-items: center;
}
h3 .el-icon-warning-outline {
  margin-right: 8px;
  font-size: 1.2em;
}

/* Element Plus 表格样式覆盖 */
.alert-table {
  background-color: #172A45; /* 表格背景 */
  border: 1px solid #173A5E;
}

:deep(.el-table th), :deep(.el-table tr), :deep(.el-table td) {
  background-color: transparent !important;
  color: #E0E0E0 !important;
  border-bottom: 1px solid #2A3F54; /* 分隔线颜色 */
}

:deep(.el-table th) {
  color: #8892B0 !important; /* 表头文字颜色 */
  font-weight: bold;
}

:deep(.el-table--enable-row-hover .el-table__body tr:hover > td) {
  background-color: #2A3F54 !important; /* 悬停行背景色 */
}

:deep(.el-table .el-table__cell) {
    padding: 8px 0; /* 单元格内边距 */
}

:deep(.el-tag) {
    border-radius: 4px;
}
</style>

