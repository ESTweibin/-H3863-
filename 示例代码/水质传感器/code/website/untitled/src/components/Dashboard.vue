<template>
  <div class="dashboard-container">
    <header class="header">
      <h1>水上福寿螺监测与清除系统</h1>
    </header>
    <main class="main-content">
      <div class="left-panel">
        <WaterQuality class="card top-left" />
        <SnailComparisonChart class="card bottom-left" />
      </div>
      <div class="center-panel">
        <MapDisplay class="card map-area" />
      </div>
      <div class="right-panel">
        <WeatherInfo class="card top-right" />
        <AnomalyAlerts class="card bottom-right" />
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
import WaterQuality from './WaterQuality.vue';
import MapDisplay from './MapDisplay.vue';
import WeatherInfo from './WeatherInfo.vue';
import SnailComparisonChart from './SnailComparisonChart.vue';
import AnomalyAlerts from './AnomalyAlerts.vue';
</script>

<style scoped>
/* Styles are identical to the original App.vue, just moved here */
.dashboard-container {
  display: flex;
  flex-direction: column;
  height: 100vh;
  background-color: #0A192F; /* 深蓝色背景 */
  color: #E0E0E0; /* 浅色文字 */
  font-family: 'Arial', sans-serif;
}

.header {
  background-color: #001E3C; /* 稍深一点的蓝色 */
  padding: 15px 20px;
  text-align: center;
  border-bottom: 1px solid #173A5E;
}

.header h1 {
  margin: 0;
  font-size: 1.8em;
  color: #64FFDA; /* 亮眼的青色标题 */
}

.main-content {
  display: grid;
  grid-template-columns: 300px 1fr 300px; /* 左中右三栏 */
  grid-template-rows: 1fr 1fr; /* 上下两行，用于左右面板 */
  gap: 15px;
  padding: 15px;
  flex-grow: 1;
  overflow: hidden; /* 防止内容溢出导致滚动条 */
}

.left-panel, .right-panel {
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.card {
  background-color: #172A45; /* 卡片背景色 */
  border-radius: 8px;
  padding: 15px;
  box-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
  overflow-y: auto; /* 如果卡片内容过多，允许内部滚动 */
}

.left-panel {
  grid-column: 1 / 2;
  grid-row: 1 / 3; /* 左侧面板占据两行 */
}

.center-panel {
  grid-column: 2 / 3;
  grid-row: 1 / 3; /* 中间面板占据两行 */
  display: flex; /* 确保地图组件撑满 */
}

.map-area {
  flex-grow: 1; /* 地图区域撑满中间面板 */
}

.right-panel {
  grid-column: 3 / 4;
  grid-row: 1 / 3; /* 右侧面板占据两行 */
}

@media (max-width: 1200px) {
  .main-content {
    grid-template-columns: 250px 1fr 250px;
  }
}

@media (max-width: 992px) {
  .main-content {
    grid-template-columns: 1fr;
    grid-template-rows: auto;
  }
  .left-panel, .center-panel, .right-panel {
    grid-column: 1 / 2;
    grid-row: auto;
  }
  .map-area {
    min-height: 400px;
  }
}
</style>

