# -*- coding: utf-8 -*-
"""
Users表的CRUD接口定义
"""
from flask import Blueprint, request, jsonify
from models.Users import Users, db

users_bp = Blueprint('users', __name__)

@users_bp.route('/users', methods=['GET'])
def get_users():
    """
    获取所有用户信息
    """
    users = Users.query.all()
    return jsonify([u.to_dict() for u in users])

@users_bp.route('/users/<int:user_id>', methods=['GET'])
def get_user(user_id):
    """
    获取指定用户信息
    """
    u = Users.query.get_or_404(user_id)
    return jsonify(u.to_dict())

@users_bp.route('/user/login', methods=['POST'])
def login():
    data = request.json
    email = data.get('email')
    password = data.get('password')
    if not email or not password:
        return jsonify({'error': '缺少邮箱或密码'}), 400
    user = Users.query.filter_by(email=email).first()
    if user and user.check_password(password):
#         return jsonify(user.to_dict())
        return jsonify({'success': 0})
    else:
        return jsonify({'error': '邮箱或密码错误'}), 401

@users_bp.route('/users', methods=['POST'])
def create_user():
    """
    创建新用户
    """
    data = request.json
    u = Users(
        email=data['email'],
        full_name=data.get('full_name'),
        role=data.get('role', 'admin')
    )
    u.set_password(data['password']) # 使用 set_password 处理密码
    db.session.add(u)
    db.session.commit()
    return jsonify({'user_id': u.user_id}), 201

@users_bp.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id):
    """
    更新指定用户信息
    """
    u = Users.query.get_or_404(user_id)
    data = request.json
    u.email = data.get('email', u.email)
    u.password_hash = data.get('password_hash', u.password_hash)
    u.full_name = data.get('full_name', u.full_name)
    u.role = data.get('role', u.role)
    db.session.commit()
    return jsonify({'message': 'updated'})

@users_bp.route('/users/<int:user_id>', methods=['DELETE'])
def delete_user(user_id):
    """
    删除指定用户
    """
    u = Users.query.get_or_404(user_id)
    db.session.delete(u)
    db.session.commit()
    return jsonify({'message': 'deleted'})

