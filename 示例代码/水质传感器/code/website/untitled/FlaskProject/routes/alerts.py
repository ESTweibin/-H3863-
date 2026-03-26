# -*- coding: utf-8 -*-
"""
Alerts表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.Alerts import Alerts, db

alerts_bp = Blueprint('alerts', __name__)

@alerts_bp.route('/alerts', methods=['GET'])
def get_alerts():
    """
    获取所有警报
    """
    alerts = Alerts.query.all()
    return jsonify([a.to_dict() for a in alerts])

@alerts_bp.route('/alerts/<int:alert_id>', methods=['GET'])
def get_alert(alert_id):
    """
    获取指定警报
    """
    a = Alerts.query.get_or_404(alert_id)
    return jsonify(a.to_dict())

@alerts_bp.route('/alerts', methods=['POST'])
def create_alert():
    """
    创建新警报
    """
    data = request.json
    a = Alerts(
        alert_type=data['alert_type'],
        severity=data.get('severity', 'MEDIUM'),
        snail_observation_id=data.get('snail_observation_id'),
        water_quality_reading_id=data.get('water_quality_reading_id'),
        latitude=data['latitude'],
        longitude=data['longitude'],
        alert_timestamp=data['alert_timestamp'],
        description=data.get('description'),
        status=data.get('status', 'NEW'),
        resolved_by_user_id=data.get('resolved_by_user_id'),
        resolved_at=data.get('resolved_at')
    )
    db.session.add(a)
    db.session.commit()
    return jsonify({'alert_id': a.alert_id}), 201

@alerts_bp.route('/alerts/<int:alert_id>', methods=['PUT'])
def update_alert(alert_id):
    """
    更新指定警报
    """
    a = Alerts.query.get_or_404(alert_id)
    data = request.json
    a.alert_type = data.get('alert_type', a.alert_type)
    a.severity = data.get('severity', a.severity)
    a.snail_observation_id = data.get('snail_observation_id', a.snail_observation_id)
    a.water_quality_reading_id = data.get('water_quality_reading_id', a.water_quality_reading_id)
    a.latitude = data.get('latitude', a.latitude)
    a.longitude = data.get('longitude', a.longitude)
    a.alert_timestamp = data.get('alert_timestamp', a.alert_timestamp)
    a.description = data.get('description', a.description)
    a.status = data.get('status', a.status)
    a.resolved_by_user_id = data.get('resolved_by_user_id', a.resolved_by_user_id)
    a.resolved_at = data.get('resolved_at', a.resolved_at)
    db.session.commit()
    return jsonify({'message': 'updated'})

@alerts_bp.route('/alerts/<int:alert_id>', methods=['DELETE'])
def delete_alert(alert_id):
    """
    删除指定警报
    """
    a = Alerts.query.get_or_404(alert_id)
    db.session.delete(a)
    db.session.commit()
    return jsonify({'message': 'deleted'})

