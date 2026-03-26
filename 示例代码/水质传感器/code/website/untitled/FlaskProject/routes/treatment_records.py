# -*- coding: utf-8 -*-
"""
TreatmentRecords表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.TreatmentRecords import TreatmentRecords, db

treat_bp = Blueprint('treatment_records', __name__)

@treat_bp.route('/treatment_records', methods=['GET'])
def get_treatment_records():
    """
    获取所有治理记录
    """
    trs = TreatmentRecords.query.all()
    return jsonify([t.to_dict() for t in trs])

@treat_bp.route('/treatment_records/<int:treatment_record_id>', methods=['GET'])
def get_treatment_record(treatment_record_id):
    """
    获取指定治理记录
    """
    t = TreatmentRecords.query.get_or_404(treatment_record_id)
    return jsonify(t.to_dict())

@treat_bp.route('/treatment_records', methods=['POST'])
def create_treatment_record():
    """
    创建新治理记录
    """
    data = request.json
    t = TreatmentRecords(
        treatment_date=data['treatment_date'],
        count_before_treatment=data['count_before_treatment'],
        date_of_count_before=data['date_of_count_before'],
        count_after_treatment=data.get('count_after_treatment'),
        date_of_count_after=data.get('date_of_count_after'),
        efficacy_notes=data.get('efficacy_notes')
    )
    db.session.add(t)
    db.session.commit()
    return jsonify({'treatment_record_id': t.treatment_record_id}), 201

@treat_bp.route('/treatment_records/<int:treatment_record_id>', methods=['PUT'])
def update_treatment_record(treatment_record_id):
    """
    更新指定治理记录
    """
    t = TreatmentRecords.query.get_or_404(treatment_record_id)
    data = request.json
    t.treatment_date = data.get('treatment_date', t.treatment_date)
    t.count_before_treatment = data.get('count_before_treatment', t.count_before_treatment)
    t.date_of_count_before = data.get('date_of_count_before', t.date_of_count_before)
    t.count_after_treatment = data.get('count_after_treatment', t.count_after_treatment)
    t.date_of_count_after = data.get('date_of_count_after', t.date_of_count_after)
    t.efficacy_notes = data.get('efficacy_notes', t.efficacy_notes)
    db.session.commit()
    return jsonify({'message': 'updated'})

@treat_bp.route('/treatment_records/<int:treatment_record_id>', methods=['DELETE'])
def delete_treatment_record(treatment_record_id):
    """
    删除指定治理记录
    """
    t = TreatmentRecords.query.get_or_404(treatment_record_id)
    db.session.delete(t)
    db.session.commit()
    return jsonify({'message': 'deleted'})

