# -*- coding: utf-8 -*-
"""
SnailObservations表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.SnailObservations import SnailObservations, db

snail_bp = Blueprint('snail_observations', __name__)

@snail_bp.route('/snail_observations', methods=['GET'])
def get_snail_observations():
    """
    获取所有福寿螺观测记录
    """
    obs = SnailObservations.query.all()
    return jsonify([o.to_dict() for o in obs])

@snail_bp.route('/snail_observations/<int:observation_id>', methods=['GET'])
def get_snail_observation(observation_id):
    """
    获取指定观测记录
    """
    o = SnailObservations.query.get_or_404(observation_id)
    return jsonify(o.to_dict())

@snail_bp.route('/snail_observations', methods=['POST'])
def create_snail_observation():
    """
    创建新观测记录，支持图片上传
    """
    if request.content_type and request.content_type.startswith('multipart/form-data'):
        data = request.form
        file = request.files.get('image')
        notes = data.get('notes', '')
        image_path = r'D:\wu_lian_wang'
        if file:
            import os
            from werkzeug.utils import secure_filename
            # upload_dir = os.path.join('static', 'uploads')
            # os.makedirs(upload_dir, exist_ok=True)
            os.makedirs(image_path, exist_ok=True)
            filename = secure_filename(file.filename)
            image_path = os.path.join(image_path, filename)
            file.save(image_path)
            # 保存相对路径到notes
            image_url = '' + image_path.replace('\\', '/')
            notes = (notes + '\n' if notes else '') + f'{image_url}'
        o = SnailObservations(
            latitude=data['latitude'],
            longitude=data['longitude'],
            timestamp=data['timestamp'],
            apple_snail_count=data.get('apple_snail_count', 0),
            notes=notes
        )
    else:
        data = request.json
        o = SnailObservations(
            latitude=data['latitude'],
            longitude=data['longitude'],
            timestamp=data['timestamp'],
            apple_snail_count=data.get('apple_snail_count', 0),
            notes=data.get('notes')
        )
    db.session.add(o)
    db.session.commit()
    return jsonify({'observation_id': o.observation_id}), 201

@snail_bp.route('/snail_observations/<int:observation_id>', methods=['PUT'])
def update_snail_observation(observation_id):
    """
    更新指定观测记录
    """
    o = SnailObservations.query.get_or_404(observation_id)
    data = request.json
    o.latitude = data.get('latitude', o.latitude)
    o.longitude = data.get('longitude', o.longitude)
    o.timestamp = data.get('timestamp', o.timestamp)
    o.apple_snail_count = data.get('apple_snail_count', o.apple_snail_count)
    o.notes = data.get('notes', o.notes)
    db.session.commit()
    return jsonify({'message': 'updated'})

@snail_bp.route('/snail_observations/<int:observation_id>', methods=['DELETE'])
def delete_snail_observation(observation_id):
    """
    删除指定观测记录
    """
    o = SnailObservations.query.get_or_404(observation_id)
    db.session.delete(o)
    db.session.commit()
    return jsonify({'message': 'deleted'})

