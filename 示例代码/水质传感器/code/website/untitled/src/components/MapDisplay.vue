<template>
  <div class="map-container">
    <div id="amap-container" style="width: 100%; height: 100%;"></div>
    <div class="map-controls">
      <el-button type="primary" @click="toggleSnailMarkers">{{ showSnailMarkers ? '隐藏' : '显示' }}福寿螺位置</el-button>
      <el-button type="primary" @click="toggleHeatmap">{{ showHeatmap ? '隐藏' : '显示' }}福寿螺热力图</el-button>
      <el-button type="primary" @click="toggleTrack">{{ showTrack ? '隐藏' : '显示' }}船行轨迹</el-button>
    </div>
    <el-dialog v-model="imageDialogVisible" title="福寿螺图片" width="30%">
      <img :src="currentSnailImage" alt="福寿螺" style="width: 100%;" />
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, shallowRef } from 'vue';
import AMapLoader from '@amap/amap-jsapi-loader';
import { ElButton, ElDialog } from 'element-plus';
// import { getSnailObservations } from '/snail_observations'; // 导入获取福寿螺观测数据的接口
// import { getLatestVoyage } from '@/api/voyages'; // 导入获取最新航行记录的接口
// import { getVoyagePathPoints } from '@/api/voyage_path_points'; // 导入获取航行路径点的接口
import {getSnailObservations} from "../api/snail_observations.ts";
import {getLatestVoyage} from "../api/voyages.ts";
import {getVoyagePathPoints} from "../api/voyage_path_points.ts";
const map = shallowRef<any>(null);
let amapInstance: any = null;
const showSnailMarkers = ref(false);
const showHeatmap = ref(false);
const showTrack = ref(false);
const imageDialogVisible = ref(false);
const currentSnailImage = ref('');

let snailPositions = ref<any[]>([]); // 用于存储从API获取的福寿螺位置数据
let trackPath = ref<any[]>([]); // 用于存储从API获取的船行轨迹数据

let snailMarkers: any[] = [];
let heatmapLayer: any = null;
let trackPolyline: any = null;
let boatMarker: any = null;

const fetchSnailData = async () => {
  try {
    const response = await getSnailObservations();
    snailPositions.value = response.data.map((obs: any) => ({
      lnglat: [obs.longitude, obs.latitude],
      count: obs.apple_snail_count,
      image: obs.notes || '', // 假设图片URL在notes字段，如果没有则为空字符串
      observation_id: obs.observation_id
    }));

    if (snailPositions.value.length > 0) {
      const latestSnail = snailPositions.value[snailPositions.value.length - 1];
      if (map.value) {
        map.value.setCenter(latestSnail.lnglat);
      }
    }
  } catch (error) {
    console.error('获取福寿螺数据失败:', error);
  }
};

const fetchTrackData = async () => {
  try {
    const latestVoyageResponse = await getLatestVoyage();
    if (latestVoyageResponse.data && latestVoyageResponse.data.voyage_id) {
      const voyageId = latestVoyageResponse.data.voyage_id;
      const pathPointsResponse = await getVoyagePathPoints();
      trackPath.value = pathPointsResponse.data
        .filter((point: any) => point.voyage_id === voyageId)
        .map((point: any) => [point.longitude, point.latitude])
        .sort((a: any, b: any) => {
          return new Date(a.timestamp).getTime() - new Date(b.timestamp).getTime();
        });

      if (trackPath.value.length > 0 && boatMarker) {
        boatMarker.setPosition(trackPath.value[0]);
      }
    }
  } catch (error) {
    console.error('获取船行轨迹数据失败:', error);
  }
};

onMounted(async () => {
  await fetchSnailData(); // 获取福寿螺数据
  await fetchTrackData(); // 获取船行轨迹数据

  AMapLoader.load({
    // key: '7433d4a6577ebe66b04e6889315290bf',
    key: '38ef875432accb5b354b077738ca271f',
    version: '2.0',
    plugins: ['AMap.HeatMap', 'AMap.MoveAnimation'],
  })
    .then((AMap) => {
      amapInstance = AMap;
      let initialCenter = [116.404, 39.915];
      if (snailPositions.value.length > 0) {
        initialCenter = snailPositions.value[snailPositions.value.length - 1].lnglat;
      }

      map.value = new AMap.Map('amap-container', {
        zoom: 18,
        center: initialCenter,
        mapStyle: 'amap://styles/white',
      });

      const initialBoatPosition = trackPath.value.length > 0 ? trackPath.value[0] : initialCenter;
      boatMarker = new AMap.Marker({
        position: initialBoatPosition,
        offset: new AMap.Pixel(0, 0),
        // offset: new AMap.Pixel(-13, -26),
      });
      map.value.add(boatMarker);
    })
    .catch((e) => {
      console.error('高德地图加载失败:', e);
    });
});

const toggleSnailMarkers = () => {
  showSnailMarkers.value = !showSnailMarkers.value;
  if (showSnailMarkers.value) {
    if (!amapInstance || !map.value || snailPositions.value.length === 0) return;
    snailMarkers = snailPositions.value.map(pos => {
      const marker = new amapInstance.Marker({
        position: pos.lnglat,
        title: `福寿螺数量: ${pos.count}`
      });
      console.log(marker);
      marker.on('click', () => {
        currentSnailImage.value = pos.image;
        imageDialogVisible.value = true;
      });
      return marker;
    });
    map.value.add(snailMarkers);
  } else {
    if (map.value && snailMarkers.length > 0) {
      map.value.remove(snailMarkers);
      snailMarkers = [];
    }
  }
};

const toggleHeatmap = () => {
  showHeatmap.value = !showHeatmap.value;
  if (showHeatmap.value) {
    if (!amapInstance || !map.value || snailPositions.value.length === 0) return;
    if (!heatmapLayer) {
      heatmapLayer = new amapInstance.HeatMap(map.value, {
        radius: 25,
        opacity: [0, 0.8],
        gradient: {
          0.5: 'blue',
          0.65: 'rgb(117,211,248)',
          0.7: 'rgb(0, 255, 0)',
          0.9: '#ffea00',
          1.0: 'red'
        }
      });
    }
    const heatmapData = snailPositions.value.map(p => ({ lng: p.lnglat[0], lat: p.lnglat[1], count: p.count }));
    heatmapLayer.setDataSet({ data: heatmapData, max: Math.max(...heatmapData.map(d => d.count), 10) });
    heatmapLayer.show();
  } else {
    if (heatmapLayer) {
      heatmapLayer.hide();
    }
  }
};

const toggleTrack = () => {
  showTrack.value = !showTrack.value;
  if (showTrack.value) {
    if (!amapInstance || !map.value || trackPath.value.length === 0) return;
    if (!trackPolyline) {
      trackPolyline = new amapInstance.Polyline({
        path: trackPath.value,
        strokeColor: '#28F',
        strokeOpacity: 0.8,
        strokeWeight: 6,
        strokeStyle: 'solid',
      });
      console.log(trackPolyline);
    }
    map.value.add(trackPolyline);
    if (boatMarker && trackPath.value.length > 0) {
      boatMarker.setPosition(trackPath.value[0]);
      boatMarker.moveAlong(trackPath.value, {
        duration: 20000,
        autoRotation: true,
      });
    }
  } else {
    if (map.value && trackPolyline) {
      map.value.remove(trackPolyline);
    }
  }
};

onUnmounted(() => {
  if (map.value) {
    map.value.destroy();
  }
});
</script>

<style scoped>
.map-container {
  position: relative;
  width: 100%;
  height: 100%;
}
.map-controls {
  position: absolute;
  bottom: 20px;
  left: 50%;
  transform: translateX(-50%);
  background-color: rgba(23, 42, 69, 0.9); /* 深蓝透明背景 */
  padding: 10px;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.3);
  display: flex;
  gap: 10px;
  z-index: 10;
}
/* Element Plus 按钮样式调整 */
:deep(.el-button--primary) {
  background-color: #007BFF; /* Element Plus 默认蓝色 */
  border-color: #007BFF;
  color: #FFF;
}
:deep(.el-button--primary:hover) {
  background-color: #0056b3;
  border-color: #0056b3;
}
:deep(.el-dialog) {
    background-color: #0A192F; /* 对话框背景 */
    color: #E0E0E0;
}
:deep(.el-dialog__title) {
    color: #64FFDA; /* 对话框标题颜色 */
}
:deep(.el-dialog__headerbtn .el-dialog__close) {
    color: #E0E0E0; /* 关闭按钮颜色 */
}
</style>

