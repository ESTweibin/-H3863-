import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// 水质读数相关接口
export const getWaterQualityReadings = async () => axios.get(`${API_BASE}/water_quality_readings`);
export const getWaterQualityReading = (id: number) => axios.get(`/water_quality_readings/${id}`);
export const createWaterQualityReading = (data: any) => axios.post('/water_quality_readings', data);
export const updateWaterQualityReading = (id: number, data: any) => axios.put(`/water_quality_readings/${id}`, data);
export const deleteWaterQualityReading = (id: number) => axios.delete(`/water_quality_readings/${id}`);
export const getWaterQualityReadingsLatest = async () => axios.get(`${API_BASE}/water_quality_readings/latest`);
