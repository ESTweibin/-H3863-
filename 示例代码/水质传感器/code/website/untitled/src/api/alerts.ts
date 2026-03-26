import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// Alerts 警报相关接口
export const getAlerts = async () => axios.get(`${API_BASE}/alerts`);
export const getAlert = (id: number) => axios.get(`${API_BASE}/alerts/${id}`);
export const createAlert = (data: any) => axios.post(`${API_BASE}/alerts`, data);
export const updateAlert = (id: number, data: any) => axios.put(`${API_BASE}/alerts/${id}`, data);
export const deleteAlert = (id: number) => axios.delete(`${API_BASE}/alerts/${id}`);

