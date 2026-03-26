# -*- coding: utf-8 -*-
"""
WaterQualityReadings表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.WaterQualityReadings import WaterQualityReadings, db

wq_bp = Blueprint('water_quality_readings', __name__)

@wq_bp.route('/water_quality_readings/latest', methods=['GET'])
def get_latest_water_quality_reading():
    """
    获取最新的水质读数
    """
    # 按 timestamp 降序排列，获取第一条记录
    # 如果您希望按 reading_id 排序，可以使用 WaterQualityReadings.reading_id.desc()
    latest_reading = WaterQualityReadings.query.order_by(WaterQualityReadings.timestamp.desc()).first()
    if latest_reading:
        return jsonify(latest_reading.to_dict())
    else:
        return jsonify({'message': 'No water quality readings found'}), 404


@wq_bp.route('/water_quality_readings', methods=['GET'])
def get_water_quality_readings():
    """
    获取所有水质读数
    """
    readings = WaterQualityReadings.query.all()
    return jsonify([r.to_dict() for r in readings])

@wq_bp.route('/water_quality_readings/<int:reading_id>', methods=['GET'])
def get_water_quality_reading(reading_id):
    """
    获取指定水质读数
    """
    r = WaterQualityReadings.query.get_or_404(reading_id)
    return jsonify(r.to_dict())

@wq_bp.route('/water_quality_readings', methods=['POST'])
def create_water_quality_reading():
    """
    创建新水质读数
    """
    data = request.json
    r = WaterQualityReadings(
        latitude=data['latitude'],
        longitude=data['longitude'],
        timestamp=data['timestamp'],
        temperature_celsius=data.get('temperature_celsius'),
        ph_value=data.get('ph_value'),
        dissolved_oxygen_mgl=data.get('dissolved_oxygen_mgl'),
        turbidity_ntu=data.get('turbidity_ntu'),
        conductivity_us_cm=data.get('conductivity_us_cm'),
        source=data.get('source', 'manual'),
        notes=data.get('notes')
    )
    db.session.add(r)
    db.session.commit()
    return jsonify({'reading_id': r.reading_id}), 201

@wq_bp.route('/water_quality_readings/<int:reading_id>', methods=['PUT'])
def update_water_quality_reading(reading_id):
    """
    更新指定水质读数
    """
    r = WaterQualityReadings.query.get_or_404(reading_id)
    data = request.json
    r.latitude = data.get('latitude', r.latitude)
    r.longitude = data.get('longitude', r.longitude)
    r.timestamp = data.get('timestamp', r.timestamp)
    r.temperature_celsius = data.get('temperature_celsius', r.temperature_celsius)
    r.ph_value = data.get('ph_value', r.ph_value)
    r.dissolved_oxygen_mgl = data.get('dissolved_oxygen_mgl', r.dissolved_oxygen_mgl)
    r.turbidity_ntu = data.get('turbidity_ntu', r.turbidity_ntu)
    r.conductivity_us_cm = data.get('conductivity_us_cm', r.conductivity_us_cm)
    r.source = data.get('source', r.source)
    r.notes = data.get('notes', r.notes)
    db.session.commit()
    return jsonify({'message': 'updated'})

@wq_bp.route('/water_quality_readings/<int:reading_id>', methods=['DELETE'])
def delete_water_quality_reading(reading_id):
    """
    删除指定水质读数
    """
    r = WaterQualityReadings.query.get_or_404(reading_id)
    db.session.delete(r)
    db.session.commit()
    return jsonify({'message': 'deleted'})

