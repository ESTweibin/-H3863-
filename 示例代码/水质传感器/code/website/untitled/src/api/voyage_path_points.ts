import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// 航行路径点相关接口
export const getVoyagePathPoints = () => axios.get(`${API_BASE}/voyage_path_points`);
export const getVoyagePathPoint = (id: number) => axios.get(`${API_BASE}/voyage_path_points/${id}`);
export const createVoyagePathPoint = (data: any) => axios.post(`${API_BASE}/voyage_path_points`, data);
export const updateVoyagePathPoint = (id: number, data: any) => axios.put(`${API_BASE}/voyage_path_points/${id}`, data);
export const deleteVoyagePathPoint = (id: number) => axios.delete(`${API_BASE}/voyage_path_points/${id}`);

