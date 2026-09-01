all:
	protoc -I="." --cpp_out="." ./if_proto_common.proto
	protoc -I="." --cpp_out="." ./if_drc_drsu.proto
	protoc -I="." --cpp_out="." ./bxt.obstacle_v2.proto
	protoc -I="." --cpp_out="." ./bxt.common.Header.proto
	protoc -I="." --cpp_out="." ./bxt.common.math.MatrixXd.proto
	protoc -I="." --cpp_out="." ./bxt.common.sensor.SensorSource.proto
	protoc -I="." --cpp_out="." ./bxt.common.math.Vector3d.proto
	protoc -I="." --cpp_out="." ./bxt.common.math.point.proto
	protoc -I="." --cpp_out="." ./bxt.StatusPb.proto
	protoc -I="." --cpp_out="." ./log_level.proto
	protoc -I="." --cpp_out="." ./thread_cpucore_cfg.proto
	protoc -I="." --cpp_out="." ./if_drsu_geofencing.proto
	protoc -I="." --cpp_out="." ./drc_event.proto
	
	protoc -I="." --cpp_out="." ./kafka_config.proto
	protoc -I="." --cpp_out="." ./bxt_mqtt_config.proto
	protoc -I="." --cpp_out="." ./bxt_if_cns.proto
	protoc -I="." --cpp_out="." ./bxt.uav.proto
	protoc -I="." --cpp_out="." ./local_cfg.proto
	protoc -I="." --cpp_out="." ./bxt_if_uav.proto
	protoc -I="." --cpp_out="." ./dji_cloud_api.proto
	protoc -I="." --cpp_out="." ./bxt_cloud_common.proto
	protoc -I="." --cpp_out="." ./dji_dock3_osd.proto

	#protoc -I="." --cpp_out="." ./http_client_cfg.proto
	#protoc -I="." --cpp_out="." ./bxt_if_ladar.proto
	#protoc -I="." --cpp_out="." ./cmlc_comm_5ga.proto
	#protoc -I="." --cpp_out="." ./sense_data_5g_a.proto

	# protoc -I="." --cpp_out="." ./pad_msg.proto
	# protoc -I="." --cpp_out="." ./chassis.proto
	# protoc -I="." --cpp_out="." ./control_command.proto
	# protoc -I="." --cpp_out="." ./if_drc_fusion.proto
	# protoc -I="." --cpp_out="." ./config_file.proto
	# protoc -I="." --cpp_out="." ./bxt.PointUTM.proto
	# protoc -I="." --cpp_out="." ./if_platform_stream.proto
	# protoc -I="." --cpp_out="." ./drc_log_config.proto	
	# protoc -I="." --cpp_out="." ./drc_event_debug.proto
	# protoc -I="." --cpp_out="." ./if_drc_crm_r4.proto
	# protoc -I="." --cpp_out="." ./if_drc_acu.proto
	# protoc -I="." --cpp_out="." ./if_broadxt_zebra.proto
	# protoc -I="." --cpp_out="." ./bxt_cns_id.proto
	# protoc -I="." --cpp_out="." ./cns_nacos_config.proto
	
clean:
	-rm -f *.cc *.h
