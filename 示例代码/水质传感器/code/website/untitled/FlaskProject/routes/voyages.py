# -*- coding: utf-8 -*-
"""
Voyages表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.Voyages import Voyages, db

voyages_bp = Blueprint('voyages', __name__)


@voyages_bp.route('/voyages', methods=['GET'])
def get_voyages():
    """
    获取所有航行记录
    """
    voyages = Voyages.query.all()
    return jsonify([v.to_dict() for v in voyages])


@voyages_bp.route('/voyages/latest', methods=['GET'])
def get_latest_voyage():
    """
    获取最新的航行记录
    """
    # 按 voyage_id 降序排列，获取最新的一个
    # 如果没有记录，first() 会返回 None
    latest_voyage = Voyages.query.order_by(Voyages.voyage_id.desc()).first()
    if latest_voyage:
        return jsonify(latest_voyage.to_dict())
    else:
        return jsonify({'message': 'No voyages found'}), 404

@voyages_bp.route('/voyages/<int:voyage_id>', methods=['GET'])
def get_voyage(voyage_id):
    """
    获取指定航行记录
    """
    v = Voyages.query.get_or_404(voyage_id)
    return jsonify(v.to_dict())


@voyages_bp.route('/voyages', methods=['POST'])
def create_voyage():
    """
    创建新航行记录
    """
    data = request.json
    v = Voyages(
        user_id=data.get('user_id'),
        vessel_identifier=data.get('vessel_identifier'),
        start_timestamp=data['start_timestamp'],
        end_timestamp=data.get('end_timestamp'),
        purpose=data.get('purpose'),
        notes=data.get('notes')
    )
    db.session.add(v)
    db.session.commit()
    return jsonify({'voyage_id': v.voyage_id}), 201


@voyages_bp.route('/voyages/<int:voyage_id>', methods=['PUT'])
def update_voyage(voyage_id):
    """
    更新指定航行记录
    """
    v = Voyages.query.get_or_404(voyage_id)
    data = request.json
    v.user_id = data.get('user_id', v.user_id)
    v.vessel_identifier = data.get('vessel_identifier', v.vessel_identifier)
    v.start_timestamp = data.get('start_timestamp', v.start_timestamp)
    v.end_timestamp = data.get('end_timestamp', v.end_timestamp)
    v.purpose = data.get('purpose', v.purpose)
    v.notes = data.get('notes', v.notes)
    db.session.commit()
    return jsonify({'message': 'updated'})


@voyages_bp.route('/voyages/<int:voyage_id>', methods=['DELETE'])
def delete_voyage(voyage_id):
    """
    删除指定航行记录
    """
    v = Voyages.query.get_or_404(voyage_id)
    db.session.delete(v)
    db.session.commit()
    return jsonify({'message': 'deleted'})
