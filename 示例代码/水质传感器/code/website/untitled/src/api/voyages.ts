import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// 航行记录相关接口
export const getVoyages = () => axios.get(`${API_BASE}/voyages`);
export const getLatestVoyage = async () => axios.get(`${API_BASE}/voyages/latest`);
export const getVoyage = (id: number) => axios.get(`${API_BASE}/voyages/${id}`);
export const createVoyage = (data: any) => axios.post(`${API_BASE}/voyages`, data);
export const updateVoyage = (id: number, data: any) => axios.put(`${API_BASE}/voyages/${id}`, data);
export const deleteVoyage = (id: number) => axios.delete(`${API_BASE}/voyages/${id}`);

