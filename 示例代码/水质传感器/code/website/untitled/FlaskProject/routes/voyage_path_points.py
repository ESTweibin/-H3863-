# -*- coding: utf-8 -*-
"""
VoyagePathPoints表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.VoyagePathPoints import VoyagePathPoints, db

vpp_bp = Blueprint('voyage_path_points', __name__)

@vpp_bp.route('/voyage_path_points', methods=['GET'])
def get_voyage_path_points():
    """
    获取所有航行路径点
    """
    points = VoyagePathPoints.query.all()
    return jsonify([p.to_dict() for p in points])



@vpp_bp.route('/voyage_path_points/<int:point_id>', methods=['GET'])
def get_voyage_path_point(point_id):
    """
    获取指定路径点
    """
    p = VoyagePathPoints.query.get_or_404(point_id)
    return jsonify(p.to_dict())

@vpp_bp.route('/voyage_path_points', methods=['POST'])
def create_voyage_path_point():
    """
    创建新路径点
    """
    data = request.json
    p = VoyagePathPoints(
        voyage_id=data['voyage_id'],
        latitude=data['latitude'],
        longitude=data['longitude'],
        altitude=data.get('altitude'),
        speed=data.get('speed'),
        timestamp=data['timestamp']
    )
    db.session.add(p)
    db.session.commit()
    return jsonify({'point_id': p.point_id}), 201

@vpp_bp.route('/voyage_path_points/<int:point_id>', methods=['PUT'])
def update_voyage_path_point(point_id):
    """
    更新指定路径点
    """
    p = VoyagePathPoints.query.get_or_404(point_id)
    data = request.json
    p.voyage_id = data.get('voyage_id', p.voyage_id)
    p.latitude = data.get('latitude', p.latitude)
    p.longitude = data.get('longitude', p.longitude)
    p.altitude = data.get('altitude', p.altitude)
    p.speed = data.get('speed', p.speed)
    p.timestamp = data.get('timestamp', p.timestamp)
    db.session.commit()
    return jsonify({'message': 'updated'})

@vpp_bp.route('/voyage_path_points/<int:point_id>', methods=['DELETE'])
def delete_voyage_path_point(point_id):
    """
    删除指定路径点
    """
    p = VoyagePathPoints.query.get_or_404(point_id)
    db.session.delete(p)
    db.session.commit()
    return jsonify({'message': 'deleted'})

