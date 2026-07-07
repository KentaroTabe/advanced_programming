#!/bin/sh
# imagemagickで何か画像処理をして，/imgprocにかきこみ，テンプレートマッチング
# 最終テスト時は、直下の for を for image in $1/final/*.ppm; do に変更してください
for image in $1/test/*.ppm; do
    bname=$(basename "${image}")
    name="imgproc/${bname}"
    x=0
    echo "$name"
    magick "${image}" "${name}"  # 何もしない画像処理

    echo "${bname}:"
    
    for template in $1/*.ppm; do
        # 該当ディレクトリに ppm ファイルが存在しない場合のエラー回避
        [ -e "$template" ] || continue

        tname=$(basename "${template}")
        echo "Template: $tname"

        # 1. 回転のループ
        for rotation in 0 90 180 270; do
            
            # 2. 拡大・縮小のループ
            for size in 50 100 200; do
                size_str="${size}%"
                
                final_template="imgproc/rot${rotation}_scale${size}_${tname}"
                
                if [ "$size" -lt 100 ]; then
                    threshold=1.6
                    magick "${template}" -rotate $rotation -fuzz 5% -fill black -opaque black -statistic Median 2 -scale "${size_str}" "${final_template}"
                
                elif [ "$size" -eq 100 ]; then
                    threshold=0.8
                    magick "${template}" -rotate $rotation -fuzz 5% -fill black -opaque black -scale "${size_str}" "${final_template}"
                
                else
                    threshold=0.3
                    magick "${template}" -rotate $rotation -fuzz 5% -fill black -opaque black -scale "${size_str}" "${final_template}"
                fi

                mkdir -p imgproc/tmp
                temp_template="imgproc/tmp/${tname}"
                cp "${final_template}" "${temp_template}"

                # マッチングの実行と出力の取得
                if [ $x -eq 0 ]; then
                    OUTPUT=$(./matching "$name" "${temp_template}" $rotation $threshold cp)
                    x=1
                else
                    OUTPUT=$(./matching "$name" "${temp_template}" $rotation $threshold p)
                fi
                
                echo "$OUTPUT"

                # 出力文字列に "[Found" が含まれているか判定
                if echo "$OUTPUT" | grep -q "\[Found"; then
                    echo "マッチしたため、この画像に対する以降の探索をスキップします。"
                    # 3階層のループ（size, rotation, template）を抜けて次の image へ
                    break 3
                fi
            done
        done
    done
    echo ""
done
wait